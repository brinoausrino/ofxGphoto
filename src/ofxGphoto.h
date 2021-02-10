#pragma once

/*
 ofxGphoto lets OF talk to gphoto compatible cameras using a simple interface.

 It reuses a lot of code from ofxEdsdk by Kyle McDonald
 */

#include <gphoto2/gphoto2.h>
#include "ofMain.h"
#include "RateTimer.h"
#include "FixedQueue.h"
#include "FreeImage.h"
#include "GphotoHelperFunctions.h"

namespace ofxGphoto {

	struct PhotoData{
		const char *ptr;
		unsigned long int size;
		CameraFile *file;
	};

	struct CameraInformation{
		int id;
		string name;
		string port;
		string serialNumber;
	};
	
	class GPhoto : public ofThread {
	public:
		GPhoto();
		vector<CameraInformation> listDevices() const;
		//void setDeviceId(int deviceId);
        void setOrientationMode(int orientationMode);
        void setLiveView(bool useLiveView);
		bool isLiveView();
		void setup();
		void setup(int id);
		void setup(string cameraName);
        bool close();
		~GPhoto();
        
		void update();
		bool isFrameNew();
		unsigned int getWidth() const;
		unsigned int getHeight() const;
		bool isLiveDataReady() const;
		void draw(float x, float y);
        void draw(float x, float y, float width, float height);
        const ofPixels& getLivePixels() const;
        const ofTexture& getLiveTexture() const;
		float getFrameRate();
        float getBandwidth();
        
		void takePhoto(bool blocking = false);
		bool isPhotoNew();
		void drawPhoto(float x, float y);
		void drawPhoto(float x, float y, float width, float height);
        bool savePhoto(string filename); // .jpg only
        const ofPixels& getPhotoPixels() const;
        const ofTexture& getPhotoTexture() const;

        bool isConnected() { return connected; }
        
	private:
		void initialize(int id);
        void startCapture();
        void captureLoop();
        void stopCapture();
		
		RateTimer fps;
        float bytesPerFrame;
		
		/*
		 Live view data is read from the camera into liveBufferBack when DownloadEvfData()
		 is called. Then the class is locked, and liveBufferBack is quickly pushed
		 onto the liveBufferMiddle queue. When update() is called, the class is
		 also locked to quickly pop from liveBufferMiddle into liveBufferFront.
		 At this point, the pixels are decoded into livePixels and uploaded to liveTexture.
		 */
		//bool isLiveBufferFilled = false;
		ofBuffer* liveBufferBack;
		FixedQueue<ofBuffer*> liveBufferMiddle;
		ofBuffer* liveBufferFront;
		mutable ofPixels livePixels;
		mutable ofTexture liveTexture;

		vector<FIBITMAP*> buffers;
		
		/*
		 Photo data is read from the camera into photoBuffer when downloadImage() is
		 called after being triggered by takePhoto(). photoBuffer is only decoded
		 into photoPixels when getPhotoPixels() is called. drawPhoto() will call
		 getPhotoPixels(), and also upload photoPixels to photoTexture. savePhoto()
		 does not decode photoBuffer.
		 */
		ofBuffer *photoBuffer;
		mutable ofPixels photoPixels;
		mutable ofTexture photoTexture;
		
		/*
		 There are a few important state variables used for keeping track of what
		 is and isn't ready, and syncing data between the main thread and the
		 capture thread.
		 */
		bool connected; // camera is valid, OpenSession was successful, you can use Eds(camera) now.
        bool useLiveView; // Whether to initialize live view on setup().
		bool liveDataReady; // Live view data has been downloaded at least once by threadedFunction().
		bool frameNew; // There has been a new frame since the user last checked isFrameNew().
		bool needToTakePhoto; // threadedFunction() should take a picture next chance it gets.
		bool photoNew; // There is a new photo since the user last checked isPhotoNew().
		mutable bool needToDecodePhoto; // The photo pixels needs to be decoded from photo buffer.
		mutable bool needToUpdatePhoto; // The photo texture needs to be updated from photo pixels.
		bool photoDataReady; // Photo data has been downloaded at least once.
		bool needToSendKeepAlive; // Send keepalive next chance we get.
		bool needToDownloadImage; // Download image next chance we get.
        
		void threadedFunction();
        		
		// the liveview needs to be reset every so often to avoid the camera turning off
		float resetIntervalMinutes;
		float lastResetTime;
		//void resetLiveView();
		
		
		void setLiveViewReady(bool liveViewReady);

		void setSendKeepAlive();
		

        
        int deviceId;
        int orientationMode;

		// gphoto
		Camera	*camera;
		GPContext *cameracontext;

		PhotoData liveData;
		PhotoData photoData;

		bool updateLiveView(Camera *camera, GPContext *cameracontext,ofBuffer *buffer);
		bool shootAndDownloadPhoto(Camera *camera, GPContext *cameracontext,ofBuffer *buffer);
	};
}
