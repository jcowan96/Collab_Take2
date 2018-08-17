# Collab_Take2

To migrate these c++ classes to whatever unreal project you want to work on:
https://answers.unrealengine.com/questions/206693/how-can-i-export-class-file-to-another-project.html 

-Copy the files to the Source directory in the new project

-In the header files, rename the *_API macro to whatever the new project is called

-Add the existing items to the Visual Studio project

-Rebuild the project (Changing all the file extensions to .txt to make sure everything compiles in visual studio)


You will have to change the GMSEC_API_PATH in the Build.cs script, and possibly the "server" variable in SessionAdvertiser.h, SessionSeeker.h, SessionSlaveNode.h, and SessionMasterNode.h, depending on where you are hosting the GMSEC bus. 
