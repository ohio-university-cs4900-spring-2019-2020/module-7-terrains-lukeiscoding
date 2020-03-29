#include "GLViewNewModule.h"

#include "WorldList.h" //This is where we place all of our WOs
#include "ManagerOpenGLState.h" //We can change OpenGL State attributes with this
#include "Axes.h" //We can set Axes to on/off with this
#include "PhysicsEngineODE.h"

//Different WO used by this module
#include "WO.h"
#include "WOStatic.h"
#include "WOStaticPlane.h"
#include "WOStaticTrimesh.h"
#include "WOTrimesh.h"
#include "WOHumanCyborg.h"
#include "WOHumanCal3DPaladin.h"
#include "WOWayPointSpherical.h"
#include "WOLight.h"
#include "WOSkyBox.h"
#include "WOCar1970sBeater.h"
#include "Camera.h"
#include "CameraStandard.h"
#include "CameraChaseActorSmooth.h"
#include "CameraChaseActorAbsNormal.h"
#include "CameraChaseActorRelNormal.h"
#include "Model.h"
#include "ModelDataShared.h"
#include "ModelMesh.h"
#include "ModelMeshDataShared.h"
#include "ModelMeshSkin.h"
#include "WONVStaticPlane.h"
#include "WONVPhysX.h"
#include "WONVDynSphere.h"
#include "AftrGLRendererBase.h"

//If we want to use way points, we need to include this.
#include "NewModuleWayPoints.h"
//NetMsg
#include "NetMessengerClient.h"
#include "NetMsgSimpleWO.h"
//PhysX
#include "PhysXModule.h"
//fonts
#include "WOGUILabel.h"
#include "WOFTGLString.h"
#include "MGLFTGLString.h"
//terrain
#include "WOGridECEFElevation.h"
#include "WOPhysXTriangularMesh.h"
#include <vector>


using namespace Aftr;
using namespace physx;




GLViewNewModule* GLViewNewModule::New(const std::vector< std::string >& args)
{
	//PhysX
	PhysXModule::init();
	GLViewNewModule* glv = new GLViewNewModule(args);
	glv->init(Aftr::GRAVITY, Vector(0, 0, -1.0f), "aftr.conf", PHYSICS_ENGINE_TYPE::petODE);
	glv->onCreate();
	return glv;
}


GLViewNewModule::GLViewNewModule(const std::vector< std::string >& args) : GLView(args)
{
	//Initialize any member variables that need to be used inside of LoadMap() here.
	//Note: At this point, the Managers are not yet initialized. The Engine initialization
	//occurs immediately after this method returns (see GLViewNewModule::New() for
	//reference). Then the engine invoke's GLView::loadMap() for this module.
	//After loadMap() returns, GLView::onCreate is finally invoked.

	//The order of execution of a module startup:
	//GLView::New() is invoked:
	//    calls GLView::init()
	//       calls GLView::loadMap() (as well as initializing the engine's Managers)
	//    calls GLView::onCreate()

	//GLViewNewModule::onCreate() is invoked after this module's LoadMap() is completed.
}


void GLViewNewModule::onCreate()
{
	//GLViewNewModule::onCreate() is invoked after this module's LoadMap() is completed.
	//At this point, all the managers are initialized. That is, the engine is fully initialized.

	if (this->pe != NULL)
	{
		//optionally, change gravity direction and magnitude here
		//The user could load these values from the module's aftr.conf
		this->pe->setGravityNormalizedVector(Vector(0, 0, -1.0f));
		this->pe->setGravityScalar(Aftr::GRAVITY);
	}
	this->setActorChaseType(STANDARDEZNAV); //Default is STANDARDEZNAV mode
	//this->setNumPhysicsStepsPerRender( 0 ); //pause physics engine on start up; will remain paused till set to 1

	// play the background sound
	SoundManager::init();
	SoundManager::playBackGround("../../../shared/mm/sounds/space.ogg", true, false, true);
	SoundManager::sound2D.at(0)->setVolume(0.5f);

	//Network
	if (ManagerEnvironmentConfiguration::getVariableValue("NetServerListenPort") == "8889") {
		client = NetMessengerClient::New("127.0.0.1", "8888");
	}
	else {
		client = NetMessengerClient::New("127.0.0.1", "8889");
	}
}



GLViewNewModule::~GLViewNewModule()
{
	//Implicitly calls GLView::~GLView()
}


void GLViewNewModule::updateWorld()
{
	GLView::updateWorld(); //Just call the parent's update world first.
						   //If you want to add additional functionality, do it after
						   //this call.
	// update cam position
	SoundManager::setCamPosition(this->cam->getPosition(), this->cam->getLookDirection(), Vector(0, 0, 0), this->cam->getNormalDirection());

	// pos update // it works but not useful, too much waste.
	//WO* wo = actorLst->at(0);
	//NetMsgSimpleWO msg;
	//msg.pos = wo->getPosition();
	//msg.id = 0;
	//client->sendNetMsgSynchronousTCP(msg);

	//PhysX
	PhysXModule::scene->simulate(ManagerSDLTime::getTimeSinceLastPhysicsIteration() / 1000.0f);
	PhysXModule::scene->fetchResults(true);
	PxU32 numActors = 0;
	PxActor** actors = PhysXModule::scene->getActiveActors(numActors);
	for (PxU32 i = 0; i < numActors; i++) {
		PxActor* actor = actors[i];
		PxRigidActor* a = static_cast<PxRigidActor*>(actor);
		PxTransform t = a->getGlobalPose();
		PxMat44 td = PxMat44(t);
		float dmaf[16] = {td(0,0),td(0,1),td(0,2),td(3,0),td(1,0),td(1,1),td(1,2),td(3,1),td(2,0),td(2,1),td(2,2),td(3,2),td(0,3),td(1,3),td(2,3),td(3,3)};
		Mat4 dma(dmaf);
		WO* awo = static_cast<WO*>(a->userData);
		awo->getModel()->setDisplayMatrix(dma);
		awo->setPosition(dma[12], dma[13], dma[14]);

		Vector pos = awo->getPosition();
		pos.y = pos.y + 5;
		int id = actorLst->getIndexOfWO(awo) + 1;
		WOFTGLString* wostring = (WOFTGLString*)actorLst->at(id);
		wostring->getModel()->setDisplayMatrix(dma);
		wostring->setPosition(pos);

		NetMsgSimpleWO msg;
		msg.pos = awo->getPosition();
		msg.dma = dma;
		msg.id = actorLst->getIndexOfWO(awo) / 2;
		msg.new_indicator = 0;
		if (client->isTCPSocketOpen()) {
			client->sendNetMsgSynchronousTCP(msg);
		}
	}

}


void GLViewNewModule::onResizeWindow(GLsizei width, GLsizei height)
{
	GLView::onResizeWindow(width, height); //call parent's resize method.
}


void GLViewNewModule::onMouseDown(const SDL_MouseButtonEvent& e)
{
	GLView::onMouseDown(e);
}


void GLViewNewModule::onMouseUp(const SDL_MouseButtonEvent& e)
{
	GLView::onMouseUp(e);
}


void GLViewNewModule::onMouseMove(const SDL_MouseMotionEvent& e)
{
	GLView::onMouseMove(e);
}

static int speech_check = 1;

void GLViewNewModule::onKeyDown(const SDL_KeyboardEvent& key)
{
	GLView::onKeyDown(key);
	if (key.keysym.sym == SDLK_0)
		this->setNumPhysicsStepsPerRender(1);

	// press 's' to open your sound radar to determine where the enermy is
	// the enermy will say different things. Listen carefully
	if (key.keysym.sym == SDLK_s) 
	{
		if (speech_check == 1) 
		{
			cout << "Enermy voice detected!!!" << endl;
			Vector cur_cam_pos = this->cam->getPosition();
			SoundManager::play3DSound("../../../shared/mm/sounds/voice/WOHuman Your Teacher Looks Funny.wav", Vector(0, 0, 5), false, false, true);
			SoundManager::sound3D.back()->setVolume(0.9f);
			SoundManager::sound3D.back()->setMinDistance(10.0f);
			speech_check = 0;
		}
		else
		{
			cout << "Enermy is talking !!!" << endl;
			Vector cur_cam_pos = this->cam->getPosition();
			SoundManager::play3DSound("../../../shared/mm/sounds/voice/WOHuman Making Me Laugh.wav", Vector(0, 0, 5), false, false, true);
			SoundManager::sound3D.back()->setVolume(0.9f);
			SoundManager::sound3D.back()->setMinDistance(10.0f);
			speech_check = 1;
		}
	}



	//// press '1' to take off the jet 1
	//if (key.keysym.sym == SDLK_1)
	//{
	//	WO* wo = actorLst->at(0);
	//	Vector v = wo->getPosition();
	//	if (v.x > 30) {
	//		wo->moveRelative(Vector(0.5, 0.5, 0.5));
	//		wo->resetJoints();
	//		wo->changeOrientationWRTparentDeltaRoll(10);
	//	}
	//	else {
	//		wo->moveRelative(Vector(0.5, 0.5, 0));
	//	}
	//	printf("jet 1 taking off!\n");
	//	// update position
	//	NetMsgSimpleWO msg;
	//	msg.pos = wo->getPosition();
	//	msg.id = 0;
	//	client->sendNetMsgSynchronousTCP(msg);


	//}
	//// press '2' to take off the jet 2
	//if (key.keysym.sym == SDLK_2)
	//{
	//	WO* wo = actorLst->at(1);
	//	Vector v = wo->getPosition();
	//	if (v.x > 30) {
	//		wo->moveRelative(Vector(0.5, -0.5, 0.6));
	//		wo->resetJoints();
	//		wo->changeOrientationWRTparentDeltaRoll(10);
	//	}
	//	else {
	//		wo->moveRelative(Vector(0.5, -0.5, 0));
	//	}
	//	printf("jet 2 taking off!\n");
	//	// update position
	//	NetMsgSimpleWO msg;
	//	msg.pos = wo->getPosition();
	//	msg.id = 1;
	//	client->sendNetMsgSynchronousTCP(msg);
	//}
	// press '3' to take off the jet 3
	if (key.keysym.sym == SDLK_3 )
	{
		std::string jet(ManagerEnvironmentConfiguration::getSMM() + "/models/jet_wheels_down_PP.wrl");
		PxMaterial* gMaterial = PhysXModule::gPhysics->createMaterial(0.5f, 0.5f, 0.6f);
		WO* wo1 = WO::New(jet, Vector(1, 1, 1), MESH_SHADING_TYPE::mstFLAT);
		wo1->setPosition(Vector(10, 0, 50));
		wo1->renderOrderType = RENDER_ORDER_TYPE::roOPAQUE;
		wo1->setLabel("jetnew");
		worldLst->push_back(wo1);

		Vector dropPos1 = wo1->getPosition();
		PxTransform t1 = PxTransform(PxVec3(dropPos1.x, dropPos1.y, dropPos1.z));
		PxShape* shape1 = PhysXModule::gPhysics->createShape(PxBoxGeometry(3.0f, 3.0f, 3.0f), *gMaterial);
		PxRigidDynamic* actor1 = PxCreateDynamic(*PhysXModule::gPhysics, t1, *shape1, 1.0f);
		PhysXModule::addActor(wo1, actor1); // Physx knows its aftr counterpart
		printf("jet new taking off!\n");
		 //update position
		actorLst->push_back(wo1);
		NetMsgSimpleWO msg;
		msg.pos = wo1->getPosition();
		msg.id = actorLst->getIndexOfWO(wo1) / 2;
		msg.new_indicator = 1;
		if (client->isTCPSocketOpen()) {
			client->sendNetMsgSynchronousTCP(msg);
		}

		string jet_name = "jet " + std::to_string(msg.id);
		WOFTGLString* wostring = WOFTGLString::New(ManagerEnvironmentConfiguration::getSMM() + "/fonts/COMIC.TTF", 30);
		wostring->getModelT<MGLFTGLString>()->setFontColor(aftrColor4f(1.0f, 1.0f, 1.0f, 1.0f));
		wostring->getModelT<MGLFTGLString>()->setSize(5, 5);
		wostring->getModelT<MGLFTGLString>()->setText(jet_name);
		wostring->rotateAboutGlobalX(PI / 2);
		wostring->rotateAboutGlobalZ(-PI / 2);
		Vector pos = wo1->getPosition();
		pos.y = pos.y + 5;
		wostring->setPosition(pos);
		wostring->setLabel("String");
		worldLst->push_back(wostring);
		actorLst->push_back(wostring);
	}

	// press 'e' to change sound effct
	// it will crash!!!

	//if (key.keysym.sym == SDLK_e)
	//{
	//	SoundManager::changeEffect();
	//}

	// press 'c' to show the car
	// the enermy will say different things. Listen carefully
	//if (key.keysym.sym == SDLK_c)
	//{
	//	//std::string racecar("../mm/models/spider.obj");
	//	std::string racecar("../mm/models/formula_1/Formula_1_mesh.obj");

	//	cout << "A Ferrari Occoured!!!!" << endl;
	//	WO* wo = WO::New(racecar, Vector(0.025, 0.025, 0.025), MESH_SHADING_TYPE::mstFLAT);
	//	wo->setPosition(Vector(35, 0, 1.5));
	//	wo->renderOrderType = RENDER_ORDER_TYPE::roOPAQUE;
	//	wo->setLabel("racecar");
	//	worldLst->push_back(wo);
	//}
}


void GLViewNewModule::onKeyUp(const SDL_KeyboardEvent& key)
{
	GLView::onKeyUp(key);
}


void Aftr::GLViewNewModule::loadMap()
{
	this->worldLst = new WorldList(); //WorldList is a 'smart' vector that is used to store WO*'s
	this->actorLst = new WorldList();
	this->netLst = new WorldList();

	ManagerOpenGLState::GL_CLIPPING_PLANE = 1000.0;
	ManagerOpenGLState::GL_NEAR_PLANE = 0.1f;
	ManagerOpenGLState::enableFrustumCulling = false;
	Axes::isVisible = true;
	this->glRenderer->isUsingShadowMapping(false); //set to TRUE to enable shadow mapping, must be using GL 3.2+

	this->cam->setPosition(15, 15, 10);

	std::string shinyRedPlasticCube(ManagerEnvironmentConfiguration::getSMM() + "/models/cube4x4x4redShinyPlastic_pp.wrl");
	std::string wheeledCar(ManagerEnvironmentConfiguration::getSMM() + "/models/rcx_treads.wrl");
	std::string grass(ManagerEnvironmentConfiguration::getSMM() + "/models/grassFloor400x400_pp.wrl");
	std::string human(ManagerEnvironmentConfiguration::getSMM() + "/models/human_chest.wrl");
	std::string jet(ManagerEnvironmentConfiguration::getSMM() + "/models/jet_wheels_down_PP.wrl");

	//SkyBox Textures readily available
	std::vector< std::string > skyBoxImageNames; //vector to store texture paths
	//skyBoxImageNames.push_back( ManagerEnvironmentConfiguration::getSMM() + "/images/skyboxes/sky_water+6.jpg" );
	//skyBoxImageNames.push_back( ManagerEnvironmentConfiguration::getSMM() + "/images/skyboxes/sky_dust+6.jpg" );
	//skyBoxImageNames.push_back( ManagerEnvironmentConfiguration::getSMM() + "/images/skyboxes/sky_mountains+6.jpg" );
	//skyBoxImageNames.push_back( ManagerEnvironmentConfiguration::getSMM() + "/images/skyboxes/sky_winter+6.jpg" );
	//skyBoxImageNames.push_back( ManagerEnvironmentConfiguration::getSMM() + "/images/skyboxes/early_morning+6.jpg" );
	//skyBoxImageNames.push_back( ManagerEnvironmentConfiguration::getSMM() + "/images/skyboxes/sky_afternoon+6.jpg" );
	//skyBoxImageNames.push_back( ManagerEnvironmentConfiguration::getSMM() + "/images/skyboxes/sky_cloudy+6.jpg" );
	//skyBoxImageNames.push_back( ManagerEnvironmentConfiguration::getSMM() + "/images/skyboxes/sky_cloudy3+6.jpg" );
	//skyBoxImageNames.push_back( ManagerEnvironmentConfiguration::getSMM() + "/images/skyboxes/sky_day+6.jpg" );
	//skyBoxImageNames.push_back( ManagerEnvironmentConfiguration::getSMM() + "/images/skyboxes/sky_day2+6.jpg" );
	//skyBoxImageNames.push_back( ManagerEnvironmentConfiguration::getSMM() + "/images/skyboxes/sky_deepsun+6.jpg" );
	//skyBoxImageNames.push_back( ManagerEnvironmentConfiguration::getSMM() + "/images/skyboxes/sky_evening+6.jpg" );
	//skyBoxImageNames.push_back( ManagerEnvironmentConfiguration::getSMM() + "/images/skyboxes/sky_morning+6.jpg" );
	//skyBoxImageNames.push_back( ManagerEnvironmentConfiguration::getSMM() + "/images/skyboxes/sky_morning2+6.jpg" );
	//skyBoxImageNames.push_back( ManagerEnvironmentConfiguration::getSMM() + "/images/skyboxes/sky_noon+6.jpg" );
	//skyBoxImageNames.push_back( ManagerEnvironmentConfiguration::getSMM() + "/images/skyboxes/sky_warp+6.jpg" );
	//skyBoxImageNames.push_back( ManagerEnvironmentConfiguration::getSMM() + "/images/skyboxes/space_Hubble_Nebula+6.jpg" );
	//skyBoxImageNames.push_back( ManagerEnvironmentConfiguration::getSMM() + "/images/skyboxes/space_gray_matter+6.jpg" );
	//skyBoxImageNames.push_back( ManagerEnvironmentConfiguration::getSMM() + "/images/skyboxes/space_easter+6.jpg" );
	//skyBoxImageNames.push_back( ManagerEnvironmentConfiguration::getSMM() + "/images/skyboxes/space_hot_nebula+6.jpg" );
	//skyBoxImageNames.push_back( ManagerEnvironmentConfiguration::getSMM() + "/images/skyboxes/space_ice_field+6.jpg" );
	//skyBoxImageNames.push_back( ManagerEnvironmentConfiguration::getSMM() + "/images/skyboxes/space_lemon_lime+6.jpg" );
	//skyBoxImageNames.push_back( ManagerEnvironmentConfiguration::getSMM() + "/images/skyboxes/space_milk_chocolate+6.jpg" );
	skyBoxImageNames.push_back(ManagerEnvironmentConfiguration::getSMM() + "/images/skyboxes/space_solar_bloom+6.jpg");
	//skyBoxImageNames.push_back( ManagerEnvironmentConfiguration::getSMM() + "/images/skyboxes/space_thick_rb+6.jpg" );

	float ga = 0.1f; //Global Ambient Light level for this module
	ManagerLight::setGlobalAmbientLight(aftrColor4f(ga, ga, ga, 1.0f));
	WOLight* light = WOLight::New();
	light->isDirectionalLight(true);
	light->setPosition(Vector(0, 0, 100));
	//Set the light's display matrix such that it casts light in a direction parallel to the -z axis (ie, downwards as though it was "high noon")
	//for shadow mapping to work, this->glRenderer->isUsingShadowMapping( true ), must be invoked.
	light->getModel()->setDisplayMatrix(Mat4::rotateIdentityMat({ 0, 1, 0 }, 90.0f * Aftr::DEGtoRAD));
	light->setLabel("Light");
	worldLst->push_back(light);

	//Create the SkyBox
	WO* wo_sky = WOSkyBox::New(skyBoxImageNames.at(0), this->getCameraPtrPtr());
	wo_sky->setPosition(Vector(0, 0, 0));
	wo_sky->setLabel("Sky Box");
	wo_sky->renderOrderType = RENDER_ORDER_TYPE::roOPAQUE;
	worldLst->push_back(wo_sky);

	////Create the infinite grass plane (the floor)
	/*
	WO* wo0 = WO::New(grass, Vector(1, 1, 1), MESH_SHADING_TYPE::mstFLAT);
	wo0->setPosition(Vector(0, 0, 0));
	wo0->renderOrderType = RENDER_ORDER_TYPE::roOPAQUE;
	ModelMeshSkin& grassSkin = wo0->getModel()->getModelDataShared()->getModelMeshes().at(0)->getSkins().at(0);
	grassSkin.getMultiTextureSet().at(0)->setTextureRepeats(5.0f);
	grassSkin.setAmbient(aftrColor4f(0.4f, 0.4f, 0.4f, 1.0f)); //Color of object when it is not in any light
	grassSkin.setDiffuse(aftrColor4f(1.0f, 1.0f, 1.0f, 1.0f)); //Diffuse color components (ie, matte shading color of this object)
	grassSkin.setSpecular(aftrColor4f(0.4f, 0.4f, 0.4f, 1.0f)); //Specular color component (ie, how "shiney" it is)
	grassSkin.setSpecularCoefficient(10); // How "sharp" are the specular highlights (bigger is sharper, 1000 is very sharp, 10 is very dull)
	wo0->setLabel("Grass");
	worldLst->push_back(wo0);

	PxMaterial* gMaterial = PhysXModule::gPhysics->createMaterial(0.5f, 0.5f, 0.6f);
	PxRigidStatic* groundPlane = PxCreatePlane(*PhysXModule::gPhysics, PxPlane(0, 0, 1, 0), *gMaterial);
	PhysXModule::addActor(wo0, groundPlane);
	*/

	cout << "Enter pilot name here.." << endl;
	char name[20];
	string name_string = "";
	while (name_string == "") {
		cin.getline(name, 20, '\n');
		name_string = name;
		//getline(cin, input);
	}

	WOGUILabel* label = WOGUILabel::New(nullptr);
	label->setText("Pilot: "+name_string);
	label->setColor(255, 0, 0, 255);
	label->setFontSize(30); //font size is correlated with world size
	label->setPosition(Vector(0, 1, 0));
	label->setFontOrientation(FONT_ORIENTATION::foLEFT_TOP);
	label->setFontPath(ManagerEnvironmentConfiguration::getSMM() + "/fonts/TIMES.TTF");
	worldLst->push_back(label);


	////Create the infinite grass plane that uses the Open Dynamics Engine (ODE)
	//wo = WOStatic::New( grass, Vector(1,1,1), MESH_SHADING_TYPE::mstFLAT );
	//((WOStatic*)wo)->setODEPrimType( ODE_PRIM_TYPE::PLANE );
	//wo->setPosition( Vector(0,0,0) );
	//wo->renderOrderType = RENDER_ORDER_TYPE::roOPAQUE;
	//wo->getModel()->getModelDataShared()->getModelMeshes().at(0)->getSkins().at(0).getMultiTextureSet().at(0)->setTextureRepeats( 5.0f );
	//wo->setLabel( "Grass" );
	//worldLst->push_back( wo );

	////Create the infinite grass plane that uses NVIDIAPhysX(the floor)
	//wo = WONVStaticPlane::New( grass, Vector(1,1,1), MESH_SHADING_TYPE::mstFLAT );
	//wo->setPosition( Vector(0,0,0) );
	//wo->renderOrderType = RENDER_ORDER_TYPE::roOPAQUE;
	//wo->getModel()->getModelDataShared()->getModelMeshes().at(0)->getSkins().at(0).getMultiTextureSet().at(0)->setTextureRepeats( 5.0f );
	//wo->setLabel( "Grass" );
	//worldLst->push_back( wo );

	////Create the infinite grass plane (the floor)
	//wo = WONVPhysX::New( shinyRedPlasticCube, Vector(1,1,1), MESH_SHADING_TYPE::mstFLAT );
	//wo->setPosition( Vector(0,0,50.0f) );
	//wo->renderOrderType = RENDER_ORDER_TYPE::roOPAQUE;
	//wo->setLabel( "Grass" );
	//worldLst->push_back( wo );

	//wo = WONVPhysX::New( shinyRedPlasticCube, Vector(1,1,1), MESH_SHADING_TYPE::mstFLAT );
	//wo->setPosition( Vector(0,0.5f,75.0f) );
	//wo->renderOrderType = RENDER_ORDER_TYPE::roOPAQUE;
	//wo->setLabel( "Grass" );
	//worldLst->push_back( wo );

	//wo = WONVDynSphere::New( ManagerEnvironmentConfiguration::getVariableValue("sharedmultimediapath") + "/models/sphereRp5.wrl", Vector(1.0f, 1.0f, 1.0f), mstSMOOTH );
	//wo->setPosition( 0,0,100.0f );
	//wo->setLabel( "Sphere" );
	//this->worldLst->push_back( wo );

	//wo = WOHumanCal3DPaladin::New( Vector( .5, 1, 1 ), 100 );
	//((WOHumanCal3DPaladin*)wo)->rayIsDrawn = false; //hide the "leg ray"
	//((WOHumanCal3DPaladin*)wo)->isVisible = false; //hide the Bounding Shell
	//wo->setPosition( Vector(20,20,20) );
	//wo->setLabel( "Paladin" );
	//worldLst->push_back( wo );
	//actorLst->push_back( wo );
	//netLst->push_back( wo );
	//this->setActor( wo );
	//
	//wo = WOHumanCyborg::New( Vector( .5, 1.25, 1 ), 100 );
	//wo->setPosition( Vector(20,10,20) );
	//wo->isVisible = false; //hide the WOHuman's bounding box
	//((WOHuman*)wo)->rayIsDrawn = false; //show the 'leg' ray
	//wo->setLabel( "Human Cyborg" );
	//worldLst->push_back( wo );
	//actorLst->push_back( wo ); //Push the WOHuman as an actor
	//netLst->push_back( wo );
	//this->setActor( wo ); //Start module where human is the actor

	////Create and insert the WOWheeledVehicle
	//std::vector< std::string > wheels;
	//std::string wheelStr( "../../../shared/mm/models/WOCar1970sBeaterTire.wrl" );
	//wheels.push_back( wheelStr );
	//wheels.push_back( wheelStr );
	//wheels.push_back( wheelStr );
	//wheels.push_back( wheelStr );
	//wo = WOCar1970sBeater::New( "../../../shared/mm/models/WOCar1970sBeater.wrl", wheels );
	//wo->setPosition( Vector( 5, -15, 20 ) );
	//wo->setLabel( "Car 1970s Beater" );
	//((WOODE*)wo)->mass = 200;
	//worldLst->push_back( wo );
	//actorLst->push_back( wo );
	//this->setActor( wo );
	//netLst->push_back( wo );

	createGrid();
	//createNewModuleWayPoints();
}


void GLViewNewModule::createNewModuleWayPoints()
{
	// Create a waypoint with a radius of 3, a frequency of 5 seconds, activated by GLView's camera, and is visible.
	WayPointParametersBase params(this);
	params.frequency = 5000;
	params.useCamera = true;
	params.visible = true;
	WOWayPointSpherical* wayPt = WOWP1::New(params, 3);
	wayPt->setPosition(Vector(50, 0, 3));
	worldLst->push_back(wayPt);
}

void Aftr::GLViewNewModule::createGrid()
{

	//woodland.bmp
	double top = 34.2072593790098f;
	double bottom = 33.9980272592999f;

	double left = -118.65234375f;
	double right = -118.443603515625f;

	double vert = top - bottom;
	double horz = right - left;

	VectorD offset((top + bottom) / 2.0, (left + right) / 2.0, 0.0);
	centerOfWorld = offset.toECEFfromWGS84().toVecS();
	gravityDirection = centerOfWorld;
	gravityDirection.normalize();
	//PhysXModule::setGravity(gravityDirection);

	const int gran = 50;

	VectorD scale = VectorD(0.1f, 0.1f, 0.1f);
	//VectorD scale = VectorD(1.0f, 1.0f, 1.0f);
	VectorD upperLeft(top, left, 0);
	VectorD lowerRight(bottom, right, 0);

	//WO* grid = WOGridECEFElevation::New(upperLeft, lowerRight, gran, offset, scale, ManagerEnvironmentConfiguration::getLMM() + "/images/Woodland.tif");
	//WO* grid = WOGridECEFElevation::New(upperLeft, lowerRight, 0, offset, scale, ManagerEnvironmentConfiguration::getLMM() + "images/Woodland.tif");
	WO* grid = WOPhysXTriangularMesh::New(upperLeft, lowerRight, 0, offset, scale, ManagerEnvironmentConfiguration::getLMM() + "images/Woodland.tif");
	///*
	VectorD ll = ((upperLeft + lowerRight) / 2.0);
	ll.z = 0.0;
	VectorD zdir = ll.toECEFfromWGS84().normalizeMe();
	VectorD northPoleECEF = VectorD(90.0, 0.0, 0.0).toECEFfromWGS84();
	VectorD xdir = northPoleECEF - ll.toECEFfromWGS84();
	xdir = xdir.vectorProjectOnToPlane(zdir);
	xdir.normalize();
	VectorD ydir = zdir.crossProduct(xdir);
	ydir.normalize();
	//create the Local Tangent Plane (LTP) transformation matrix
	float localBodySpaceToLTP[16] = {
		(float)xdir.x, (float)xdir.y, (float)xdir.z, 0.0f,
		(float)ydir.x, (float)ydir.y, (float)ydir.z, 0.0f,
		(float)zdir.x, (float)zdir.y, (float)zdir.z, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};

	grid->getModel()->setDisplayMatrix(Mat4(localBodySpaceToLTP).transposeUpperLeft3x3());
	grid->setPosition(0.0f, 0.0f, -50.0f);

	grid->setLabel("grid");
	worldLst->push_back(grid);
	createTriangleMesh(grid);

	for (size_t i = 0; i < grid->getModel()->getModelDataShared()->getModelMeshes().size(); i++)
		grid->getModel()->getModelDataShared()->getModelMeshes().at(i)->getSkin().getMultiTextureSet().at(0) = ManagerTexture::loadTexture(ManagerEnvironmentConfiguration::getLMM() + "images/woodland.bmp");
	grid->getModel()->isUsingBlending(false);
}

void Aftr::GLViewNewModule::createTriangleMesh(WO * wo)
{

	// Get amount of vertices and indices

	size_t vertexListSize = wo->getModel()->getModelDataShared()->getCompositeVertexList().size();
	size_t indexListSize = wo->getModel()->getModelDataShared()->getCompositeIndexList().size();
	float* vertexListCopy = new float[vertexListSize * 3];
	unsigned int* indicesCopy = new unsigned int[indexListSize];

	// Copy the values over
	for (size_t i = 0; i < vertexListSize; i++)
	{
		vertexListCopy[i * 3 + 0] = wo->getModel()->getModelDataShared()->getCompositeVertexList().at(i).x;
		vertexListCopy[i * 3 + 1] = wo->getModel()->getModelDataShared()->getCompositeVertexList().at(i).y;
		vertexListCopy[i * 3 + 2] = wo->getModel()->getModelDataShared()->getCompositeVertexList().at(i).z;
	}
	for (size_t i = 0; i < indexListSize; i++)
		indicesCopy[i] = wo->getModel()->getModelDataShared()->getCompositeIndexList().at(i);

	// Build a new Triangle Mesh for PhysX
	PxTriangleMeshDesc meshDesc;
	meshDesc.points.count = static_cast<uint32_t>(vertexListSize);
	meshDesc.points.stride = sizeof(float) * 3;
	meshDesc.points.data = vertexListCopy;

	meshDesc.triangles.count = static_cast<uint32_t>(indexListSize) / 3;
	meshDesc.triangles.stride = 3 * sizeof(unsigned int);
	meshDesc.triangles.data = indicesCopy;

	// Cook the new mesh
	PxDefaultMemoryOutputStream writeBuffer;
	PxTriangleMeshCookingResult::Enum result;
	bool status = PhysXModule::c->cookTriangleMesh(meshDesc, writeBuffer, &result);
	if (!status)
	{
		std::cout << "Failed to create Triangular mesh" << std::endl;
		std::cin.get();
	}
	PxDefaultMemoryInputData readBuffer(writeBuffer.getData(), writeBuffer.getSize());
	PxTriangleMesh* mesh = PhysXModule::gPhysics->createTriangleMesh(readBuffer);

	// Attach a material
	PxMaterial* gMaterial = PhysXModule::gPhysics->createMaterial(0.5f, 0.5f, 0.6f);
	PxShape* shape = PhysXModule::gPhysics->createShape(PxTriangleMeshGeometry(mesh), *gMaterial, true);
	PxTransform t({ 0, 0, 0 });
	//PxTransform t({ 90, 0, 0 });

	// Attach the shape created
	Mat4 wo_pose = wo->getModel()->getDisplayMatrix();
	PxRigidStatic* actor = PhysXModule::gPhysics->createRigidStatic(t);
	bool b = actor->attachShape(*shape);
	float p[] = { wo_pose[0], wo_pose[1], wo_pose[2], wo_pose[3], wo_pose[4], wo_pose[5], wo_pose[6], wo_pose[7], wo_pose[8], wo_pose[9], wo_pose[10], wo_pose[11], wo_pose[12], wo_pose[13], wo_pose[14], wo_pose[15] };
	Vector wo_position = wo->getPosition();
	p[12] = wo_position[0];
	p[13] = wo_position[1];
	p[14] = wo_position[2];
	actor->setGlobalPose(PxTransform(PxMat44(p)));
	

	// Bind this WO, and add to the scene
	//actor->userData = wo;
	//PhysXModule::scene->addActor(*actor);

	PhysXModule::addActor(wo, actor);
}