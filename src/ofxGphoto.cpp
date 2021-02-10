#include "ofxGphoto.h"

/*
 This controls the size of liveBufferMiddle. If you are running at a low fps
 (lower than camera fps), then it will effectively correspond to the latency of the
 camera. If you're running higher than camera fps, it will determine how many frames
 you can miss without dropping one. For example, if you are running at 60 fps
 but one frame happens to last 200 ms, and your buffer size is 4, you will drop
 2 frames if your camera is running at 30 fps.
 */
#define OFX_GPHOTO_BUFFER_SIZE 1

namespace ofxGphoto {

GPhoto::GPhoto() :
	//deviceId(0),
	//orientationMode(0),
	bytesPerFrame(0),
	connected(false),
	liveDataReady(false),
	frameNew(false),
	needToTakePhoto(false),
	photoNew(false),
	useLiveView(false),
	needToDecodePhoto(false),
	needToUpdatePhoto(false),
	photoDataReady(false),
	needToSendKeepAlive(false),
	needToDownloadImage(false),
	resetIntervalMinutes(15) {
	liveBufferMiddle.resize(OFX_GPHOTO_BUFFER_SIZE);
	for(size_t i = 0; i < liveBufferMiddle.maxSize(); i++) {
		liveBufferMiddle[i] = new ofBuffer();
	}
	liveBufferFront = new ofBuffer();
	liveBufferBack = new ofBuffer();
	photoBuffer = new ofBuffer();
}

vector<CameraInformation> GPhoto::listDevices() const
{
	vector<CameraInformation> info;

	GPContext *context;
	context = gp_context_new();

	CameraList	*list;
	Camera		**cams;
	int		ret, i, count;
	const char	*name, *value;
	ret = gp_list_new (&list);

	// autodetect
	gp_list_reset (list);
	count = gp_camera_autodetect (list, context);

	if (count < GP_OK) {
		ofLogNotice("ofxGphoto::listDevices") <<"No cameras detected.";
		//return 1;
	}

	/* Now open all cameras we autodected for usage */
	ofLogNotice("ofxGphoto::listDevices") << "Number of cameras : "<< count;
	cams = (Camera**) calloc (sizeof(Camera*),count);
	for (i = 0; i < count; i++) {
		gp_list_get_name  (list, i, &name);
		gp_list_get_value (list, i, &value);

		info.push_back(CameraInformation());
		info.back().id = i;
		info.back().name = name;
		info.back().port = value;

		char 		*serial;
		ret = getConfigValueString(cams[i], "Serial Number", &serial, context);
				if (ret >= GP_OK) {
					info.back().serialNumber = serial;
					free (serial);
				}

	}
	return info;
}

	/* void Camera::setDeviceId(int deviceId) {
		this->deviceId = deviceId;
	}*/

	void GPhoto::setOrientationMode(int orientationMode) {
		this->orientationMode = orientationMode;
	}

	void GPhoto::setLiveView(bool useLiveView) {
		this->useLiveView = useLiveView;
	}

	bool GPhoto::isLiveView()
	{
		return useLiveView;
	}

	void GPhoto::setup() {
		setup(0);
	}

	void GPhoto::setup(int id)
	{
		initialize(id);
		startCapture();
		startThread();
	}

	void GPhoto::setup(string cameraName)
	{
		bool found = false;
		auto devices = listDevices();
		for (auto& d:devices){
			if(d.name == cameraName){
				found = true;
				setup(d.id);
			}
		}
		if(!found){
			ofLogNotice("ofxGphoto::setup") << "device " << cameraName << " not found. Using device with ID 0.";
			setup(0);
		}
	}

	bool GPhoto::close() {
		stopThread();
		// for some reason waiting for the thread keeps it from
		// completing, but sleeping then stopping capture is ok.
		ofSleepMillis(100);
		stopCapture();
		return true;
	}

	GPhoto::~GPhoto() {
		if(connected) {
			ofLogError() << "You must call close() before destroying the camera.";
		}
		for(size_t i = 0; i < liveBufferMiddle.maxSize(); i++) {
			delete liveBufferMiddle[i];
		}
		delete liveBufferFront;
		delete liveBufferBack;
	}

	void GPhoto::update() {
		if(connected){
			lock();
			if(liveBufferMiddle.size() > 0) {
				// decoding the jpeg in the main thread allows the capture thread to run in a tighter loop.
				swap(liveBufferFront, liveBufferMiddle.front());
				liveBufferMiddle.pop();
				unlock();
				//if ((*liveBufferBack)->data != nullptr){
				//putBmpIntoPixels(*liveBufferFront, livePixels);
				//ofLoadImage(livePixels, *liveBufferFront);
				//livePixels.rotate90(orientationMode);
				ofLoadImage(livePixels,*liveBufferFront);
				if(liveTexture.getWidth() != livePixels.getWidth() ||
						liveTexture.getHeight() != livePixels.getHeight()) {
					liveTexture.allocate(livePixels.getWidth(), livePixels.getHeight(), GL_RGB8);
				}
				liveTexture.loadData(livePixels);
				//}

				lock();
				liveDataReady = true;
				frameNew = true;
				unlock();
			} else {
				unlock();
			}
		}
	}

	bool GPhoto::isFrameNew() {
		if(frameNew) {
			frameNew = false;
			return true;
		} else {
			return false;
		}
	}

	bool GPhoto::isPhotoNew() {
		if(photoNew) {
			photoNew = false;
			return true;
		} else {
			return false;
		}
	}

	float GPhoto::getFrameRate() {
		float frameRate;
		lock();
		frameRate = fps.getFrameRate();
		unlock();
		return frameRate;
	}

	float GPhoto::getBandwidth() {
		lock();
		float bandwidth = bytesPerFrame * fps.getFrameRate();
		unlock();
		return bandwidth;
	}

	void GPhoto::takePhoto(bool blocking) {
		lock();
		needToTakePhoto = true;
		unlock();
		if(blocking) {
			while(!photoNew) {
				ofSleepMillis(10);
			}
		}
	}

	const ofPixels& GPhoto::getLivePixels() const {
		return livePixels;
	}

	const ofPixels& GPhoto::getPhotoPixels() const {
		if(needToDecodePhoto) {
			ofLoadImage(photoPixels, *photoBuffer);
			// photoPixels.rotate90(orientationMode);
			needToDecodePhoto = false;
		}
		return photoPixels;
	}

	unsigned int GPhoto::getWidth() const {
		return livePixels.getWidth();
	}
	
	unsigned int GPhoto::getHeight() const {
		return livePixels.getHeight();
	}
	
	void GPhoto::draw(float x, float y) {
		draw(x, y, getWidth(), getHeight());
	}
	
	void GPhoto::draw(float x, float y, float width, float height) {
		if(liveDataReady) {
			liveTexture.draw(x, y, width, height);
		}
	}

	const ofTexture& GPhoto::getLiveTexture() const {
		return liveTexture;
	}
	
	void GPhoto::drawPhoto(float x, float y) {
		if(photoDataReady) {
			//const ofPixels& photoPixels = getPhotoPixels();
			draw(x, y, getWidth(), getHeight());
		}
	}
	
	void GPhoto::drawPhoto(float x, float y, float width, float height) {
		if(photoDataReady) {
			getPhotoTexture().draw(x, y, width, height);
		}
	}
	
	const ofTexture& GPhoto::getPhotoTexture() const {
		if(photoDataReady) {
			const ofPixels& photoPixels = getPhotoPixels();
			if(needToUpdatePhoto) {
				if(photoTexture.getWidth() != photoPixels.getWidth() ||
						photoTexture.getHeight() != photoPixels.getHeight()) {
					photoTexture.allocate(photoPixels.getWidth(), photoPixels.getHeight(), GL_RGB8);
				}
				photoTexture.loadData(photoPixels);
				needToUpdatePhoto = false;
			}
		}
		return photoTexture;
	}

	bool GPhoto::isLiveDataReady() const {
		return liveDataReady;
	}

	void GPhoto::setSendKeepAlive() {
		lock();
		needToSendKeepAlive = true;
		unlock();
	}

	bool GPhoto::updateLiveView(Camera *camera, GPContext *cameracontext,ofBuffer *buffer)
	{
		if(connected) {
			//create new camerafile
			gp_file_new(&liveData.file);

			// get preview
			int retval = gp_camera_capture_preview(camera, liveData.file, cameracontext);
			if(retval == GP_OK) {
				//get data and size of the picture
				gp_file_get_data_and_size(liveData.file, &liveData.ptr, &liveData.size);

				// copy picture from camera to buffer
				buffer->set(liveData.ptr,liveData.size);

				// free camera data
				gp_file_unref(liveData.file);

				return true;
			}
			else {
				ofLogError("ofxGphoto") << "Getting camera preview - ERROR : "<< retval<< "  "<< gp_result_as_string(retval);
				return false;
			}
		}

		else {
			ofLogError("ofxGphoto") << "Camera is not initiated!";
			return false;
		}
		return false;
	}

	bool GPhoto::shootAndDownloadPhoto(Camera *camera, GPContext *cameracontext, ofBuffer *buffer)
	{
		if(connected) {
			//force camera to take a picture
			CameraFilePath camera_file_path;
			int retval = gp_camera_capture(camera, GP_CAPTURE_IMAGE, &camera_file_path, cameracontext);
			if(retval == GP_OK) {

				//create new camerafile
				gp_file_new(&photoData.file);

				//download picture from camera to camerafile
				retval = gp_camera_file_get(camera, camera_file_path.folder, camera_file_path.name,GP_FILE_TYPE_NORMAL, photoData.file, cameracontext);
				if(retval == GP_OK) {
					//get data and size of the picture
					gp_file_get_data_and_size(photoData.file, &photoData.ptr, &photoData.size);

					//copy picture to buffer
					buffer->set(photoData.ptr,photoData.size);

					// delete picture from camera
					retval = gp_camera_file_delete(camera, camera_file_path.folder, camera_file_path.name,cameracontext);
					if(retval != GP_OK) {
						ofLogError("ofxGphoto") << "Cannot delete picture on camera."<< " : "<< retval<< "  "<< gp_result_as_string(retval)<<endl;
					}

					// free file
					gp_file_free(photoData.file);
					return true;
				}
				else {
					ofLogError("ofxGphoto") << "Downloading camera photo - ERROR :: "<< retval<< "  "<< gp_result_as_string(retval)<<endl;
					return false;
				}
			}
			else {
				ofLogError("ofxGphoto") << "Getting camera photo - ERROR : "<< retval<< "  "<< gp_result_as_string(retval)<<endl;
				return false;
			}
		}
		else {
			ofLogError("ofxGphoto") << "Camera is not initiated!"<<endl;
			return false;
		}
	}


	bool GPhoto::savePhoto(string filename) {
		return ofBufferToFile(filename, *photoBuffer, true);
	}


	void GPhoto::initialize(int id) {
		connected = false;

		GPContext *context;
		context = gp_context_new();

		CameraList	*list;
		Camera		**cams;
		int		ret, count;
		ret = gp_list_new (&list);

		// autodetect
		gp_list_reset (list);
		count = gp_camera_autodetect (list, context);

		if (count == 0) {
			ofLogError("ofxGphoto::setup") <<"No cameras detected.";
		}else if (id >= count) {
			ofLogNotice("ofxGphoto::setup") <<"Camera id not available, taking ID 0";
			id = 0;
		}

		cams = (Camera**) calloc (sizeof(Camera*),count);
		camera = cams[id];

		cameracontext = gp_context_new();
		gp_camera_new(&camera);


		int retval = gp_camera_init(camera, cameracontext);
		if (retval != GP_OK) {
			ofLogError("ofxGphoto::setup") << "Camera initialisation error - " << retval << "   " << gp_result_as_string(retval)<<endl;
		}else {
			ofLogNotice("ofxGphoto::setup","Camera initialised successfully!");

			// get camera information
			CameraText	text;
			retval = gp_camera_get_summary(camera,&text,cameracontext);
			if (retval == GP_OK) {
				ofLogNotice("ofxGphoto::setup") << "camera information";
				ofLogNotice("=========================================");
				ofBuffer cText(text.text,30*1024);
				int count = 0;
				for(auto& l:cText.getLines()){
					ofLogNotice() << l;
					if (count>=5){
						break;
					}
					++count;
				}

			}
			else{
				ofLogError("ofxGphoto::setup") << "Failed to get Camera information - " << retval << "   " << gp_result_as_string(retval)<<endl;
			}
			connected = true;

		}
	}

	void GPhoto::startCapture() {
		setLiveView(true);
		lastResetTime = ofGetElapsedTimef();
	}

	void GPhoto::stopCapture() {
		if(connected) {
			setLiveView(false);

			int retval = gp_camera_exit(camera, cameracontext);
			if (retval != GP_OK) {
				ofLogError("ofxGphoto::setup") << "Camera disconnection error - " << retval << "   " << gp_result_as_string(retval)<<endl;
			}
			else {
				ofLogVerbose("ofxGphoto::setup","Camera disconnected successfully!");
				connected = true;

			}
		}
	}

	void GPhoto::captureLoop() {
		if(useLiveView && !needToTakePhoto) {
			if(updateLiveView(camera,cameracontext,liveBufferBack)){
				lock();
				fps.tick();
				bytesPerFrame = ofLerp(bytesPerFrame, liveBufferBack->size(), .01);
				swap(liveBufferBack, liveBufferMiddle.back());
				liveBufferMiddle.push();
				unlock();
			}
		}

		if(needToTakePhoto) {
			lock();
			shootAndDownloadPhoto(camera,cameracontext,photoBuffer);
			needToTakePhoto = false;
			photoDataReady = true;
			needToDecodePhoto = true;
			needToUpdatePhoto = true;
			needToDownloadImage = false;
			photoNew = true;
			unlock();

		}

	}

	void GPhoto::threadedFunction() {
		while(isThreadRunning()) {
			captureLoop();
			ofSleepMillis(5);
		}

	}
}
