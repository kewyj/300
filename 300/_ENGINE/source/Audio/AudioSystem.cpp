/*!*************************************************************************
****
\file		   AudioSystem.cpp
\author(s)	   Cheong Ming Lun
\par DP email: m.cheong@digipen.edu
\date		   16-8-2023
\brief

This file contains the base AudioSystem class that supports the following functionalities:
- Loading in Audio Files (from components/directory)
- Manipulating Audio (Play/Pause/Stop)
****************************************************************************/
#include "Audio/AudioSystem.h"
#include "Audio/AudioTest.h" // Scripting Tests (Audio into Lua)
#include "Audio/Audio3DTest.h"


/******************************************************************************/
/*!
	[Default Constructor] AudioSystem()
 */
 /******************************************************************************/
AudioSystem::AudioSystem() : sfxVolume(1.0f), bgmVolume(1.0f)
{
}

/******************************************************************************/
/*!
	[Destructor] ~AudioSystem()
 */
 /******************************************************************************/
AudioSystem::~AudioSystem()
{
}

/******************************************************************************/
/*!
	Init() 
	- Creates [FMOD System] 
	- Initializes [FMOD System]
	- Initalizing [Channels]
	- Populating Audio from "Entities" with <Audio> Component.
 */
 /******************************************************************************/
void AudioSystem::Init()
{
	// [Create the Audio System] -> returns the system object to this class. (&system)'
	// PINFO("FMOD System Create: %d", ErrCodeCheck(FMOD::System_Create(&system_obj)));
	PINFO("FMOD System Create: +");
	/*std::cout << "FMOD System Create:";*/
	ErrCodeCheck(FMOD::System_Create(&system_obj));

	// Initialize the System settings
	// PINFO("FMOD System Initialize: %d" , ErrCodeCheck(system_obj->init(MAX_AUDIO_FILES_PLAYING, FMOD_INIT_NORMAL, nullptr))); // Settings can be combined by doing OR operation)
	PINFO("FMOD System Initialize: +");
	ErrCodeCheck(system_obj->init(MAX_AUDIO_FILES_PLAYING, FMOD_INIT_NORMAL, nullptr));

	// Channel Initialization (SFX/BGM) - Global Volume Control
	std::vector<FMOD::Channel*> sfx_channel;
	std::vector<FMOD::Channel*> bgm_channel;

	std::vector<std::pair<uid, FMOD::Channel*&>> sfx_id_channel;
	std::vector<std::pair<uid, FMOD::Channel*&>> bgm_id_channel;

	mChannelswID.insert(std::make_pair(AUDIO_SFX, sfx_id_channel));  // w ID
	mChannelswID.insert(std::make_pair(AUDIO_BGM, bgm_id_channel));  // w ID

	auto audio_entities = systemManager->ecs->GetEntitiesWith<Audio>();

	if (audio_entities.size() > 0)
	{
		PINFO("Loading Audio Entities");
	}

	for (Entity audio : audio_entities)
	{
		Audio& audio_component = audio.GetComponent<Audio>();

		if (!audio_component.mFilePath.empty() && !audio_component.mFileName.empty()) // Accounting for empty <Audio> components.
		{
			std::string audio_path = audio_component.mFilePath + "/" + audio_component.mFileName;
			std::string audio_name = audio_component.mFileName;

			if (LoadAudio(audio_path, audio_name, &audio_component)) // (1) Load Audio + (2) Add [Sound] into Database.
			{
				audio_component.mIsLoaded = true;
				audio_component.mSound = FindSound(audio_name); // Register [Sound] reference into <Audio> component. (if successfully loaded)
			}
			else
			{
				audio_component.mIsLoaded = false;
				audio_component.mSound = nullptr;
			}
		}

		FMOD::Channel*& channel_ref = audio_component.mChannel;
		std::pair<uid, FMOD::Channel*&> channel_pair = std::make_pair(audio_component.mChannelID, std::ref(channel_ref));

		// (3) Check [AudioType] + Register <Audio> component's Channel into [SFX] / [BGM] global channel
		switch (audio_component.mAudioType)
		{
			case AUDIO_BGM:
				mChannelswID[AUDIO_BGM].push_back(channel_pair);
				break;
			case AUDIO_SFX:
				mChannelswID[AUDIO_SFX].push_back(channel_pair);
				break;
		}

	}

	// Load all Sounds
	LoadAudioFromDirectory("../assets\\Audio");

	// 3D Audio Settings
	PINFO("INITIALIZING 3D AUDIO SETTINGS:");
	ErrCodeCheck(system_obj->set3DSettings(1.0, distance_factor, 1.0)); // [Distance Factor] = 1.0f 



}

static bool testfootstep = false;
static bool testListener = false;
static bool fadeintempfix = false;
/******************************************************************************/
/*!
	Update() -> M3 going to switch to new system.
	- Iterate through <Audio> Component. 
	- Decides whether if an audio is going to be played. 
 */
 /******************************************************************************/
void AudioSystem::Update([[maybe_unused]] float dt)
{
	/* Constantly updating the[AudioListener]'s attributes based on... -> using set3DListenerAttributes()
	*  1. Player's Position in the game space.
	*  2. Camera's Direction (where it is facing)
	*/

	// Assuming that the <Listener> follows the <Camera>'s position. 
	Entity camera_ent = systemManager->ecs->GetEntitiesWith<Camera>().front();
	Transform& cam_trans = camera_ent.GetComponent<Transform>(); // In Game Camera

	auto listener_object = systemManager->ecs->GetEntitiesWith<AudioListener>();

	AudioListener* listener = nullptr;	 // There might not be an <AudioListener> 

	FMOD_VECTOR listener_pos{};
	FMOD_VECTOR velocity{};
	FMOD_VECTOR camera_forward{};
	FMOD_VECTOR up_vector{};

	if (listener_object.size() != 0)
	{
		for (Entity e : listener_object)
		{
			listener = &(e.GetComponent<AudioListener>());

			if (listener != nullptr)
			{
				listener_pos = { cam_trans.mTranslate.x , cam_trans.mTranslate.y, cam_trans.mTranslate.z };   // Constantly updating with <Camera>'s position. 

				velocity.x = (listener_pos.x - previous_position.x) / dt;
				velocity.y = (listener_pos.y - previous_position.y) / dt;
				velocity.z = (listener_pos.z - previous_position.z) / dt;

				previous_position = listener_pos;
			}
		}
	}
		
	// Do we use (1) where the camera is facing or (2) we're using a {0.0f, 0.0f, 1.0f} -> standard vector
	glm::vec3 cam_dir = systemManager->mGraphicsSystem.get()->GetCameraDirection(CAMERA_TYPE::CAMERA_TYPE_GAME);
	
	camera_forward = { cam_dir.x ,cam_dir.y , cam_dir.z };

	up_vector = { 0.0f, 1.0f, 0.0f };

	system_obj->set3DListenerAttributes(0, &listener_pos, &velocity, &camera_forward, &up_vector);      // This updates every loop. 
	 
	AudioPlayLoop(dt);

	system_obj->update();

}

/******************************************************************************/
/*!
	PlayOnAwake()
	- Checks for "mPlayonAwake" flag from <Audio> component. 
	- Plays the respective audio.
	- Linked to "Start" Button ("Resume" state)
 */
 /******************************************************************************/
void AudioSystem::PlayOnAwake()
{
	auto audio_entities = systemManager->ecs->GetEntitiesWith<Audio>();

	for (Entity audio : audio_entities)
	{
		Audio& audio_component = audio.GetComponent<Audio>();
		General& general = audio.GetComponent<General>();

		std::string name = general.name;

		if (audio_component.mPlayonAwake && audio_component.mPaused == false) // Truly the first time playing
		{
			PINFO("Playing %s on Awake", audio_component.mFileName.c_str());
			audio_component.mIsPlay = true;
		}

		if(audio_component.mPaused)
		{
			PINFO("Resuming Audio: %s.", audio_component.mFileName.c_str());
			ErrCodeCheck(audio_component.mChannel->setPaused(false));
			audio_component.mPaused = false;
			audio_component.mWasPaused = false;  // to prevent replaying of clip (if update())
			audio_component.mIsPlaying = true;
		}


		// Reload Sound
		if (!audio_component.mFilePath.empty() && !audio_component.mFileName.empty()) // Accounting for empty <Audio> components.
		{
			std::string audio_path = audio_component.mFilePath + "/" + audio_component.mFileName;
			std::string audio_name = audio_component.mFileName;

			audio_component.mIsLoaded = true;
			audio_component.mSound = FindSound(audio_name); // Register [Sound] reference into <Audio> component. (if successfully loaded)
		}
		
	}
}

/******************************************************************************/
/*!
	Pause()
	- Pauses all playing audio.
 */
 /******************************************************************************/
void AudioSystem::Pause()
{
	auto audio_entities = systemManager->ecs->GetEntitiesWith<Audio>();

	for (Entity audio : audio_entities)
	{
		Audio& audio_component = audio.GetComponent<Audio>();

		FMOD::Sound* current_sound;
		audio_component.mChannel->getCurrentSound(&current_sound);
		if (current_sound)
		{
			bool playing = false;
			audio_component.mChannel->isPlaying(&playing);
			if (playing)
			{
				PINFO("Pausing Audio : %s", audio_component.mFileName);  // I think loop too fast to display on debugger.
				ErrCodeCheck(audio_component.mChannel->setPaused(true)); // [Cannot be done in Update() loop]
				audio_component.mPaused = true;
				audio_component.mIsPlaying = false;
			}
		}
	}
}

/******************************************************************************/
/*!
	Reset()
	- Resets the audio system.
 */
 /******************************************************************************/
void AudioSystem::Reset()
{
	auto audio_entities = systemManager->ecs->GetEntitiesWith<Audio>();

	for (Entity audio : audio_entities)
	{
		Audio& audio_component = audio.GetComponent<Audio>();

		FMOD::Sound* current_sound;
		audio_component.mChannel->getCurrentSound(&current_sound);
		if (current_sound)
		{
			bool playing = false;
			audio_component.mChannel->isPlaying(&playing);
			if (playing)
			{
				ErrCodeCheck(audio_component.mChannel->stop());
			}

		}

		//audio_component.ClearAudioComponent();
		//int i = 3;
	}
}

/******************************************************************************/
/*!
	Exit()
 */
 /******************************************************************************/
void AudioSystem::Exit()
{
}

/******************************************************************************/
/*!
	[Helper Function] UpdateLoadAudio()
	- Support for editor (when a new <Audio> component has been added to an "Entity"
	[Note] : <Audio> Component must be attached to this for it to load properly.
 */
 /******************************************************************************/
void AudioSystem::UpdateLoadAudio(Entity id)
{	
	Audio& audio_component = id.GetComponent<Audio>();

	//audio_component.mFullPath = audio_component.mFilePath + "/" + audio_component.mFileName;

	if (!audio_component.mIsLoaded)
	{	
		std::string audio_path = audio_component.mFilePath;
		std::string audio_name = audio_component.mFileName;

		if (LoadAudio(audio_path, audio_name)) // What if the sound is loaded? (we have to save a reference in this function also)
		{
			audio_component.mIsLoaded = true;
			audio_component.mSound = FindSound(audio_name);  // Save a reference to the sound loaded in.
			PINFO("Audio Successfully Loaded :)");

		}
		else
		{
			audio_component.mIsLoaded = false;
			PWARNING("Audio could not be loaded... Please Check (1) Directory Name , (2) File");
		}
	}
}

/******************************************************************************/
/*!
	[Helper Function] UpdateChannelReference()
	- Support for editor (when a <Audio> component switch between 'SFX' & 'BGM' channels)
	[Note] : <Audio> Component must be attached to this for it to load properly.
 */
 /******************************************************************************/
// Used to transfer the [Channels] between [SFX] & [BGM] database.
void AudioSystem::UpdateChannelReference(Entity id)
{
	Audio& audio_component = id.GetComponent<Audio>();
	
	AUDIOTYPE type = audio_component.mAudioType;
	
	if (type != AUDIO_NULL && audio_component.mTypeChanged) // Make sure it's a valid <Audio> audiotype
	{
		switch (type)
		{
			case AUDIO_BGM:
				for (int i = 0; i < mChannelswID[AUDIO_SFX].size(); i++)
				{
					if (mChannelswID[AUDIO_SFX][i].first == audio_component.mChannelID) // if corresponding ID... is found in this database.
					{
						mChannelswID[AUDIO_SFX].erase(mChannelswID[AUDIO_SFX].begin() + i); // Delete from [BGM] database.
					}
				}

				break;
	
			case AUDIO_SFX:

				for (int i = 0; i < mChannelswID[AUDIO_BGM].size(); i++)
				{
					if (mChannelswID[AUDIO_BGM][i].first == audio_component.mChannelID) // if corresponding ID... is found in this database.
					{
						mChannelswID[AUDIO_BGM].erase(mChannelswID[AUDIO_BGM].begin() + i); // Delete from [BGM] database.
					
					}
				}

				break;
		}

		FMOD::Channel*& channel_ref = audio_component.mChannel;
		std::pair<uid, FMOD::Channel*&> channel_pair = std::make_pair(audio_component.mChannelID, std::ref(channel_ref));
		mChannelswID[type].push_back(channel_pair);
		
		// Trigger back the boolean to original.
		audio_component.mTypeChanged = false;

	}
}

/******************************************************************************/
/*!
	[Helper Function] InitAudioChannelReference()
	- Support for editor (when a <Audio> component for initializing [SFX] or [BGM] channels)
	[Note] : <Audio> Component must be attached to this for it to load properly.
 */
 /******************************************************************************/
void AudioSystem::InitAudioChannelReference(Entity id)
{
	Audio& audio_component = id.GetComponent<Audio>();

	FMOD::Channel*& channel_ref = audio_component.mChannel;
	std::pair<uid, FMOD::Channel*&> channel_pair = std::make_pair(audio_component.mChannelID, std::ref(channel_ref));

	switch (audio_component.mAudioType)
	{
		case AUDIO_BGM:
			mChannelswID[AUDIO_BGM].push_back(channel_pair);
			break;
		case AUDIO_SFX:
			mChannelswID[AUDIO_SFX].push_back(channel_pair);
			break;
	}
}

/******************************************************************************/
/*!
	[Helper Function] Update3DChannelSettings()
	- Support for editor (3D attributes for channels)
	[Note] : <Audio> Component must be attached to this for it to load properly.
 */
 /******************************************************************************/
// Updates the [3D Channel information]
void AudioSystem::Update3DChannelSettings(Entity id)
{
	Audio& audio_component = id.GetComponent<Audio>();

	FMOD_VECTOR pos = { audio_component.mPosition.x, audio_component.mPosition.y , audio_component.mPosition.z };
	FMOD_VECTOR vel = { audio_component.mVelocity.x, audio_component.mVelocity.y , audio_component.mVelocity.z };
	
	PINFO("Setting 3D Attributes for channel that exists in this <Audio> component."); 
	ErrCodeCheck(audio_component.mChannel->set3DAttributes(&pos, &vel));
}

/******************************************************************************/
/*!
	[Troubleshooter] ErrCodeCheck(FMOD_RESULT result)
	- Error Code Checker for "FMOD" related functions.
 */
 /******************************************************************************/
// Use this whenever you use a [FMOD] Functiom
int AudioSystem::ErrCodeCheck(FMOD_RESULT result)
{
	if (result != FMOD_OK)
	{
		// PWARNING("FMOD OPERATION ERROR: %d", result);
		//std::cout << "FMOD OPERATION ERROR: " << result << std::endl;
		//PINFO(" "); // concatanate nothing...

		switch (result)
		{
		case FMOD_ERR_FILE_NOTFOUND:
			PWARNING("(18) [FMOD_ERR_FILE_NOTFOUND] : The requested audio file path cannot be found, unsuccessfully loaded.");
			break;
		case FMOD_ERR_HEADER_MISMATCH:
			PWARNING("(20) [FMOD_ERR_HEADER_MISMATCH] : There is a version mismatch between the FMOD header and either the FMOD Studio library or the FMOD Low Level library.");
			//std::cout << "(20) [FMOD_ERR_HEADER_MISMATCH] : There is a version mismatch between the FMOD header and either the FMOD Studio library or the FMOD Low Level library." << std::endl;
			break;
		case FMOD_ERR_INVALID_HANDLE:
			PWARNING("(30) [FMOD_ERR_INVALID_HANDLE] : An invalid object has been passed into the FMOD function (check for nullptr)");
			break;
		}

		PWARNING("Unregistered Error Code %d", result);
		return result; // failure
	}
	PINFO("FMOD OPERATION OK.");
	//std::cout << "FMOD OPERATION OK." << std::endl;
	return 1; // success (no issues)
}


/******************************************************************************/
/*!
	 LoadAudio()
	 --------------------------------------------------------------------------
	 /brief
		- Load audio file by taking in user-defined (1) file path, (2) audio name
		- Saves a reference of the [Sound] into the <Audio> component that loads it (Editor)
	 /param
		[1] std::string file_path       - file path leading to the audio name.
		[2] std::string audio_name      - name of the audio file.
		[3] Audio*		audio_component - Pointer reference to the <Audio> component.
	 /return
		bool : Success code of the loading of audio (if it's successful or not)
 */
 /******************************************************************************/
bool AudioSystem::LoadAudio(std::string file_path, std::string audio_name, Audio* audio_component)
{
	// Check if <Audio> is already in the database.
	if (audio_component != nullptr)
	{
		auto sound_it = mSounds.find(audio_name);

		if (sound_it == mSounds.end()) // Audio not in the database.
		{
			PINFO("File Detected: %s", file_path.c_str());
			//std::cout << "File Detected: " << file_path << std::endl;
			PINFO("Creating Sound: +");
			/*std::cout << "Creating Sound: ";*/
		/*	std::string full_path = file_path + "/" + audio_name;*/
			FMOD::Sound* new_sound;
			int check = ErrCodeCheck(system_obj->createSound(audio_component->mFullPath.c_str(), FMOD_LOOP_OFF, 0, &new_sound));

			if (check != 1)
			{
				PWARNING("Error: Sound Not Loaded.");
				return 0;
				//std::cout << "Error: Sound Not Loaded." << std::endl;
			} // At this point, audio successfully loaded.

			mSounds.insert(std::make_pair(audio_name, new_sound));

			if (audio_component != nullptr) // Make sure there is an <Audio> component to save into.
			{
				audio_component->mSound = new_sound;  // Save [Sound Reference] into <Audio> component. 
				audio_component->mIsLoaded = true;
			}

			return true;
		}

		else
		{
			audio_component->mSound = FindSound(audio_name);
			audio_component->mIsLoaded = true;
			return false;
		}
	}

	return false; 
}



/******************************************************************************/
/*!
	 LoadAudioFromDirectory()
	 --------------------------------------------------------------------------
	 - Loads all audio type (.wav,.mp3 etc.) files into the local audio system
	 - This populates 'mSound' the audio database.
 */
 /******************************************************************************/
bool AudioSystem::LoadAudioFromDirectory(std::string directory_path)
{
	for (const auto& file : std::filesystem::directory_iterator(directory_path))
	{
		std::string audio_name = file.path().filename().string();

		if (!FindSound(audio_name))
		{
			PINFO("Audio Name: %s", audio_name.c_str());

			std::string file_path = file.path().string();
			PINFO("File Detected: %s", file_path.c_str());

			bool is3D = audio_name.find("3D") != std::string::npos; // Added [10/26] - label (3D) somewhere on audio file to load as a 3D audio

			FMOD_MODE mode = FMOD_DEFAULT;

			if (is3D)
			{
				mode = FMOD_3D;
			}

		
			PINFO("Creating Sound: ");
			FMOD::Sound* new_sound;
			bool check = ErrCodeCheck(system_obj->createSound(file_path.c_str(), mode , 0, &new_sound));
			

			if (!check)
			{
				PWARNING("Failed to create Sound: %s", audio_name.c_str());
				return false;
			}

			mSounds.insert(std::make_pair(audio_name, new_sound));
		}
	}

	return true;
}

/******************************************************************************/
/*!
	 PlayAudio()
	 - "Flip" the "mIsPlay" to true.
	 - AudioSystem::Update() will play once it's toggled.
 */
 /******************************************************************************/
void AudioSystem::PlayAudio(std::string audio_name,  float audio_vol, Audio* audio_component)
{
	// Find the sound first ...
	auto map_it = mSounds.find(audio_name); // tries to find the audio with this name...

	if (map_it == mSounds.end()) // true if not found;
	{
		PWARNING("Can't find the requested audio.");
		return; // ends here if [Sound] cannot be found.
	}

	if (audio_component != nullptr)
	{
		PINFO("Flipping (mIsPlay)");
		audio_component->mIsPlay = true;
		audio_component->mVolume = audio_vol;
	}
}

/******************************************************************************/
/*!
	 StopAudio()
	 - Stops playing & reset the playback to the start. (Audio Component support)
 */
 /******************************************************************************/
void AudioSystem::StopAudio(Audio* audio_component)
{
	audio_component->mChannel->stop();
	audio_component->mIsPlaying = false;
}

/******************************************************************************/
/*!
	 PauseAudio()
	 - Pause audio. (Audio Component support)
 */
 /******************************************************************************/
void AudioSystem::PauseAudio(Audio* audio_component)
{
	bool playing;
	audio_component->mChannel->isPlaying(&playing);

	if (playing)
	{
		audio_component->mChannel->setPaused(true);
		audio_component->mIsPlaying = false;
	}
}

/******************************************************************************/
/*!
	 SetAllSFXVolume()
	 - Set every SFX Channel based on the provided (1) Audio Volume
 */
 /******************************************************************************/
void AudioSystem::SetAllSFXVolume(float audio_vol)
{
	auto channel_id_it = mChannelswID.find(AUDIO_SFX);
	
	auto audio_entities = systemManager->ecs->GetEntitiesWith<Audio>();

	if (channel_id_it != mChannelswID.end())
	{
		for (auto& channel_id_pair : channel_id_it->second)
		{
			for (Entity e : audio_entities)
			{
				Audio& audio_component = e.GetComponent<Audio>();

				if (audio_component.mIsPlaying) // Only those channels which are playing can be set volume.
				{
					ErrCodeCheck(channel_id_pair.second->setVolume(audio_vol));
				}
			}
		}
	}

	sfxVolume = audio_vol;

	UpdateSFXComponentVolume(sfxVolume);
}

/******************************************************************************/
/*!
	 SetAllBGMVolume()
	 - Set every BGM Channel based on the provided (1) Audio Volume
 */
 /******************************************************************************/
void AudioSystem::SetAllBGMVolume(float audio_vol)
{
	auto channel_id_it = mChannelswID.find(AUDIO_BGM);

	auto audio_entities = systemManager->ecs->GetEntitiesWith<Audio>();

	if (channel_id_it != mChannelswID.end())
	{
		for (auto& channel_id_pair : channel_id_it->second)
		{
			for (Entity e : audio_entities)
			{
				Audio& audio_component = e.GetComponent<Audio>();

				if (audio_component.mIsPlaying) // Only those channels which are playing can be set volume.
				{
					ErrCodeCheck(channel_id_pair.second->setVolume(audio_vol));
				}
			}
		}
	}

	bgmVolume = audio_vol;

	UpdateBGMComponentVolume(bgmVolume);
}

/******************************************************************************/
/*!
	 UpdateSFXComponentVolume()
	 - Updates internal component data.
 */
 /******************************************************************************/
void AudioSystem::UpdateSFXComponentVolume(float volume)
{
	auto audio_entities = systemManager->ecs->GetEntitiesWith<Audio>();
	
	for (Entity e : audio_entities)
	{
		Audio& audio_components = e.GetComponent<Audio>();

		if(audio_components.mAudioType == AUDIO_SFX)
			audio_components.mVolume = volume;
	}
}

/******************************************************************************/
/*!
	 UpdateBGMComponentVolume()
	 - Updates internal component data.
 */
 /******************************************************************************/
void AudioSystem::UpdateBGMComponentVolume(float volume)
{
	auto audio_entities = systemManager->ecs->GetEntitiesWith<Audio>();

	for (Entity e : audio_entities)
	{
		Audio& audio_components = e.GetComponent<Audio>();

		if (audio_components.mAudioType == AUDIO_BGM)
			audio_components.mVolume = volume;
	}
}

/******************************************************************************/
/*!
	 MuteSFX()
	 - Mute all SFX Channels
 */
 /******************************************************************************/
void AudioSystem::MuteSFX()
{
	PINFO("Muting Global SFX.")
	//std::cout << "Muting Global SFX." << std::endl;
	SetAllSFXVolume(0.0f);
}

/******************************************************************************/
/*!
	 MuteBGM()
	 - Mute all BGM Channels
 */
 /******************************************************************************/
void AudioSystem::MuteBGM()
{
	PINFO("Muting Global BGM.");
	//std::cout << "Muting Global BGM." << std::endl;
	SetAllBGMVolume(0.0f);
}

/******************************************************************************/
/*!
	 StopAllSFX()
	 - Stop all SFX Channels
 */
 /******************************************************************************/
void AudioSystem::StopAllSFX()
{
	PINFO("Stopping All SFX.");

	for (auto& channel_pair : mChannelswID[AUDIO_SFX])
	{
		channel_pair.second->stop();
	}
}

/******************************************************************************/
/*!
	 StopAllBGM()
	 - Stop All BGM Channels
 */
 /******************************************************************************/
void AudioSystem::StopAllBGM()
{
	PINFO("Stopping All BGM.");
	std::cout << "Stopping All BGM." << std::endl;

	for (auto& channel_pair : mChannelswID[AUDIO_BGM])
	{
		channel_pair.second->stop();
	}
}

/******************************************************************************/
/*!
	 PauseAllSounds()
	 - Pause every channel available.
 */
 /******************************************************************************/
void AudioSystem::PauseAllSounds()
{ 
	PINFO("Pausing all Sounds.");
	auto audio_entities = systemManager->ecs->GetEntitiesWith<Audio>();

	for (Entity e : audio_entities)
	{
		Audio& audio_component = e.GetComponent<Audio>();

		if (audio_component.mIsPlaying) // Only those channels which are playing can be set volume.
		{
			audio_component.mSetPause = true;
		}
	}
}

/******************************************************************************/
/*!
	 PauseSFXSounds()
	 - Pause All sfx sounds in [SFX] channels.
 */
 /******************************************************************************/
void AudioSystem::PauseSFXSounds()
{
	for (auto& channel_pair : mChannelswID[AUDIO_SFX])
	{
		bool playing;
		channel_pair.second->isPlaying(&playing);
		if (playing)
		{
			channel_pair.second->setPaused(true);
		}
	}
}

/******************************************************************************/
/*!
	  PauseBGMSounds()
	 - Pause All bgm sounds in [BGM] channels.
 */
 /******************************************************************************/
void AudioSystem::PauseBGMSounds()
{
	for (auto& channel_pair : mChannelswID[AUDIO_BGM])
	{
		bool playing;
		channel_pair.second->isPlaying(&playing);
		if (playing)
		{
			channel_pair.second->setPaused(true);
		}
	}
}



/******************************************************************************/
/*!
	 UnpauseAllSounds()
	 - unpause every channel available.
 */
 /******************************************************************************/
void AudioSystem::UnpauseAllSounds()
{
	auto audio_entities = systemManager->ecs->GetEntitiesWith<Audio>();

	for (Entity e : audio_entities)
	{
		Audio& audio_component = e.GetComponent<Audio>();

		audio_component.mSetUnpause = true;
		
	}
}

/******************************************************************************/
/*!
	 UnpuseSFXSounds()
	 - unpause All sfx sounds in [SFX] channels.
 */
 /******************************************************************************/
void AudioSystem::UnpauseSFXSounds()
{
	for (auto& channel_pair : mChannelswID[AUDIO_SFX])
	{
		bool playing;
		channel_pair.second->isPlaying(&playing);
		if (playing)
		{
			channel_pair.second->setPaused(false);
		}
	}
}

/******************************************************************************/
/*!
	 UnpauseSFXSounds()
	 - unpause All sfx sounds in [BGM] channels.
 */
 /******************************************************************************/
void AudioSystem::UnpauseBGMSounds()
{

	for (auto& channel_pair : mChannelswID[AUDIO_BGM])
	{
		bool playing;
		channel_pair.second->isPlaying(&playing);
		if (playing)
		{
			channel_pair.second->setPaused(false);
		}
	}
}

/******************************************************************************/
/*!
	[Helper Function] FindSound()
	- Returns the sound object based on the audio name requested.
 */
 /******************************************************************************/
FMOD::Sound* AudioSystem::FindSound(std::string audio_name)
{
	auto sound_it = mSounds.find(audio_name);

	if (sound_it != mSounds.end())
	{
		return sound_it->second;
	}

	return nullptr;
	
}


/******************************************************************************/
/*!
	[Helper Function] CheckAudioExist()
	- Checks if a particular sound exists.
 */
 /******************************************************************************/
bool AudioSystem::CheckAudioExist(std::string audio_name)
{
	auto sound_it = mSounds.find(audio_name);

	if (sound_it != mSounds.end())
	{
		if (sound_it->first == audio_name) // if the audio name is found in the database.
		{	
			return true;
		}
	}

	else
	{
		return false;
	}

	return false;
}



/*
	[Check Play Loop]
	- Checks for (mIsPlay)    flag : signify that the audio is expected to play.
	- Checks for (mIsPlaying) flag : signify that it's already playing, so do not play again.
*/

void AudioSystem::AudioPlayLoop(float dt)
{
	auto audio_entities = systemManager->ecs->GetEntitiesWith<Audio>();

	for (Entity audio : audio_entities)
	{
		Audio& audio_component = audio.GetComponent<Audio>();
		// Play Cycle
		if (audio_component.mIsPlay &&							// (1) Check if this <Audio> is set to play.
			CheckAudioExist(audio_component.mFileName) &&		// (2) Check if the [Sound] that we want to play exists.
			!audio_component.mIsPlaying &&						// (3) Check if the <Audio> is currently already playing...
			!audio_component.mWasPaused &&						// (4) Check if it was [Paused] before. If yes resume.
			!audio_component.mPaused)							// (5) Cannot be paused...
		{
			PINFO("Audio Exist");
			PINFO("PLAYING AUDIO AT: %f", audio_component.mVolume);

			PlayAudioSource(audio_component, audio_component.mVolume, audio_component.m3DAudio);

			if (audio_component.m3DAudio && audio_component.mIsPlaying) // if this is a 3D audio + can only set [3D settings] if the audio is playing.
			{
				FMOD_VECTOR position = { audio_component.mPosition.x ,audio_component.mPosition.y , audio_component.mPosition.z };
				FMOD_VECTOR velocity = { audio_component.mVelocity.x ,audio_component.mVelocity.y , audio_component.mVelocity.z };

				// All this can only be set after audio has played.
				PINFO("SETTING 3D ATTRIBUTES");
				ErrCodeCheck(audio_component.mChannel->set3DAttributes(&position, &velocity)); // Need to set this for dynamic spatialization + attenuation effects.
				PINFO("SETTING 3D MIN MAX SETTINGS")
				ErrCodeCheck(audio_component.mChannel->set3DMinMaxDistance(audio_component.mMinDistance, audio_component.mMaxDistance));

				ErrCodeCheck(audio_component.mChannel->setPaused(false)); // sound is paused in "PlayAudioSource" if it's 3D audio, to setup 3D parameters first.
			}
		}

		

		// Every Loop -> check if the <Audio> is still playing.
		bool channelIsPlay = false;
		if (audio_component.mIsPlaying) // need this to be true (to indicate that there's a sound playing in the channel)
		{
			audio_component.mChannel->isPlaying(&channelIsPlay);
			audio_component.mChannel->setVolume(audio_component.mVolume);
		}


		if (audio_component.mIsPlaying && !channelIsPlay) // Sound finished playing in channel.
		{
			PINFO("Finished Playing");
			audio_component.mIsPlaying = false;	   // Audio finished playing. 
			audio_component.mIsPlay = false;       // Don't Need to keep playing... (play once) -> if "mIsLooping" is true (channel will continue to play)

			// Check if looping...
			if (audio_component.mIsLooping)
			{
				audio_component.mIsPlay = true; // make it true again. 
			}

		}

		// Fade In / Fade Out 
		if (audio_component.mFadeOut)
		{
			if (audio_component.mSound != nullptr)
			{
				if (fade_timer < audio_component.fade_duration)
				{
					float fade_step = audio_component.mFadeSpeedModifier * (audio_component.mVolume / audio_component.fade_duration);

					/*	float fade_slower_dt = 0.2 * dt;

						fade_timer += fade_slower_dt;*/

					float fadeLevelOut = audio_component.mVolume - (fade_step * dt);

					//float fadeLevelOut = audio_component.mVolume - (fade_timer / audio_component.fade_duration); //  fade level out (volume each fade out step)

					if (fadeLevelOut > audio_component.mFadeOutToVol)  // Have yet to reach the desired fade out volume
					{
						audio_component.mChannel->setVolume(fadeLevelOut);
						audio_component.mVolume = fadeLevelOut; //  fade level out
					}

					else // reached and exceeded the fade out volume...
					{
						audio_component.mChannel->setVolume(audio_component.mFadeOutToVol);
						audio_component.mVolume = audio_component.mFadeOutToVol; //  fade level out
					}

					PINFO("Fade Out Volume: %f", audio_component.mVolume);

				}
			}

			else
			{
				PINFO("Please insert your sound clip first.");
				audio_component.mFadeOut = false;
			}
		}


		if (audio_component.mFadeIn)
		{
			if (audio_component.mSound != nullptr)
			{
				if (fade_timer < audio_component.fade_duration)
				{
					float fade_step = audio_component.mFadeSpeedModifier * (audio_component.mFadeInMaxVol / audio_component.fade_duration);

					fadeLevelIn += fade_step * dt;

					if (fadeLevelIn < audio_component.mFadeInMaxVol)
					{
						if (audio_component.mIsPlaying)
						{
							audio_component.mChannel->setVolume(fadeLevelIn);
							audio_component.mVolume = fadeLevelIn;
						}

					}
					else
					{
						if (audio_component.mIsPlaying)
						{
							audio_component.mChannel->setVolume(audio_component.mFadeInMaxVol);
							audio_component.mVolume = audio_component.mFadeInMaxVol;

						}
					}

					PINFO("Fade In Volume: %f", audio_component.mVolume);

					// (1) Play Sound + Adjust Audio
					if (!audio_component.mIsPlaying)
					{
						PINFO("Playing Fade In");
						PlayAudioSource(audio_component, audio_component.mVolume);
						audio_component.mIsPlaying = true;
					}
				}
			}

			else
			{
				PINFO("Please insert your sound clip first.");
				audio_component.mFadeIn = false;
			}

		}

	}


}

/******************************************************************************/
/*!
	 [AudioSource Helper] - PlayAudioSource
	 - Funnel <Audio> component's attached [FMOD::Sound*] & [FMOD::Channel*]
 */
 /******************************************************************************/
void AudioSystem::PlayAudioSource(FMOD::Sound* comp_sound, FMOD::Channel* comp_channel, float volume)
{
	comp_channel->setVolume(volume);

	PINFO("Playing Audio Source...");
	ErrCodeCheck(system_obj->playSound(comp_sound, nullptr, false, &comp_channel));
}

void AudioSystem::PlayAudioSource(Audio& audio_component, float volume, bool audio_3d)
{
	if (audio_component.mSound != nullptr)
	{
		PINFO("Playing Audio Source from <Audio> Component");
		ErrCodeCheck(system_obj->playSound(audio_component.mSound, nullptr, audio_3d, &audio_component.mChannel));

		audio_component.mChannel->setVolume(volume); // After because in [release] mode got undefined behaviour complaining access violation. OK in Debug mode though.

		bool isPlaying; // check whether if it's playing.
		ErrCodeCheck(audio_component.mChannel->isPlaying(&isPlaying));

		if (isPlaying)
		{
			audio_component.mIsPlaying = true;
		}
	}
}
