#include "ofApp.h"

void ofApp::setup() {
	ofSetLogLevel(OF_LOG_NOTICE);
    ofSetFrameRate(60);
	ofSetVerticalSync(true);
	auto devices = camera.listDevices();

	// you can use the first camera the system finds
	camera.setup();

	// or select it by ID
	// camera.setup(1);

	// or by device name
	//camera.setup("Fuji Fujifilm X-T2");

	// start live view
	camera.setLiveView(true);
}

void ofApp::exit() {
    camera.close();
}

void ofApp::update() {
    camera.update();
	if(camera.isFrameNew()) {
		// process the live view with camera.getLivePixels()
	}
    
	if(camera.isPhotoNew()) {
		// process the photo with camera.getPhotoPixels()
		// or just save the photo to disk (jpg only):
		camera.savePhoto(ofToString(ofGetFrameNum()) + ".jpg");
	}
}

void ofApp::draw() {
	camera.draw(0, 0);
	//camera.drawPhoto(600, 0, 600, 400);

	if(camera.isLiveDataReady()) {
		stringstream status;
        status << camera.getWidth() << "x" << camera.getHeight() << " @ " <<
			(int) ofGetFrameRate() << " app-fps / " <<
			(int) camera.getFrameRate() << " cam-fps / " <<
            (camera.getBandwidth() / (1<<20)) << " MiB/s";
		ofDrawBitmapString(status.str(), 10, 20);
	}
}

void ofApp::keyPressed(int key) {
	if(key == ' ') {
		camera.takePhoto();
    }
	if(key == 'l') {
		camera.setLiveView(!camera.isLiveView());
    }
    if(key == 'c') {
        camera.close();
    }

}
