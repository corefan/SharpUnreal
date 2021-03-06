#include "SharpUnrealPrivatePCH.h"
#include "SharpUnreal.h"
#include "UnrealAPI_Engine.h"

#include <mono/jit/jit.h>
#include <mono/metadata/metadata.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/image.h>
#include <mono/metadata/class.h>
#include <mono/metadata/mono-config.h>
#include <mono/metadata/debug-helpers.h>
#include <mono/metadata/tokentype.h>
#include <mono/utils/mono-counters.h>
#include <mono/utils/mono-logger.h>
#include <mono/metadata/mono-debug.h>

//日志回调
extern "C" static void MonoPrintf(const char *string, mono_bool is_stdout)
{
	GLog->Logf(ELogVerbosity::Error, TEXT("%s"), ANSI_TO_TCHAR(string));
}

extern "C" static void MonoLog(const char *log_domain, const char *log_level, const char *message, mono_bool fatal, void *user_data)
{
	// logs are always a single line, so can avoid routing through log bridge
	// note: code is repeated because verbosity suppression is performed at compile-time
	if (fatal || 0 == FCStringAnsi::Strncmp("error", log_level, 5))
	{
		// fatal error
		GLog->Logf(ELogVerbosity::Fatal, TEXT("%s%s%s"), log_domain != nullptr ? ANSI_TO_TCHAR(log_domain) : TEXT(""), log_domain != nullptr ? TEXT(": ") : TEXT(""), ANSI_TO_TCHAR(message));
	}
	else if (0 == FCStringAnsi::Strncmp("warning", log_level, 7))
	{
		GLog->Logf(ELogVerbosity::Warning, TEXT("%s%s%s"), log_domain != nullptr ? ANSI_TO_TCHAR(log_domain) : TEXT(""), log_domain != nullptr ? TEXT(": ") : TEXT(""), ANSI_TO_TCHAR(message));
	}
	else if (0 == FCStringAnsi::Strncmp("critical", log_level, 8))
	{
		GLog->Logf(ELogVerbosity::Error, TEXT("%s%s%s"), log_domain != nullptr ? ANSI_TO_TCHAR(log_domain) : TEXT(""), log_domain != nullptr ? TEXT(": ") : TEXT(""), ANSI_TO_TCHAR(message));
	}
	else
	{
		GLog->Logf(ELogVerbosity::Log, TEXT("%s%s%s"), log_domain != nullptr ? ANSI_TO_TCHAR(log_domain) : TEXT(""), log_domain != nullptr ? TEXT(": ") : TEXT(""), ANSI_TO_TCHAR(message));
	}
}

//Log类函数注册
static void UnrealEngine_Log_Debug(MonoString* content)
{
	GLog->Logf(ELogVerbosity::Log, *FString(mono_string_to_utf16(content)));
}

static void UnrealEngine_Log_Warning(MonoString* content)
{
	GLog->Logf(ELogVerbosity::Log, *FString(mono_string_to_utf16(content)));
}

static void UnrealEngine_Log_Error(MonoString* content)
{
	GLog->Logf(ELogVerbosity::Error, *FString(mono_string_to_utf16(content)));
}

//World类函数注册
static MonoString* UnrealEngine_World_GetCurrentLevel() 
{	
	UWorld* world = GWorld.GetReference();
	if (world != NULL) 
	{
		ULevel* level = world->GetCurrentLevel();
		if (level != NULL) 
		{
			const TCHAR* name = *level->GetName();
			MonoString* ret = mono_string_from_utf16((mono_unichar2*)name);
			return ret;
		}
	}
	else {
		GLog->Log(ELogVerbosity::Error, TEXT("[World] Can't GetWorld When GetCurrentLevel."));
	}
	return NULL;
}

static void UnrealEngine_World_LoadStreamingLevel(MonoString* name) 
{
	if (name == NULL) 
	{
		GLog->Log(ELogVerbosity::Error, TEXT("[World] LoadStreamingLevel But name is Null."));
		return;
	}

	UWorld* World = GWorld.GetReference();
	if (World != NULL)
	{
		FName LevelName((const TCHAR*)mono_string_to_utf16(name));
		FLatentActionInfo LatentInfo;
		UGameplayStatics::LoadStreamLevel(World, LevelName,true,false,LatentInfo);

	}
	else {
		GLog->Log(ELogVerbosity::Error, TEXT("[World] Can't GetWorld When LoadStreamingLevel."));
	}
}

static void UnrealEngine_World_UnLoadStreamingLevel(MonoString* name)
{
	if (name == NULL)
	{
		GLog->Log(ELogVerbosity::Error, TEXT("[World] UnLoadStreamingLevel But name is Null."));
		return;
	}

	UWorld* World = GWorld.GetReference();
	if (World != NULL)
	{
		FName LevelName((const TCHAR*)mono_string_to_utf16(name));
		FLatentActionInfo LatentInfo;
		UGameplayStatics::UnloadStreamLevel(World, LevelName, LatentInfo);
	}
	else {
		GLog->Log(ELogVerbosity::Error, TEXT("[World] Can't GetWorld When UnLoadStreamingLevel."));
	}
}

static AActor* UnrealEngine_World_SpwanActor(MonoString* path, FTransform* trans)
{
	if (path == NULL)
	{
		GLog->Log(ELogVerbosity::Error, TEXT("[World] SpwanActor But path is Null."));
		return NULL;
	}
		
	if (GWorld.GetReference() == NULL)
	{
		GLog->Log(ELogVerbosity::Error, TEXT("[World] Can't GetWorld When SpwanActor."));
		return NULL;
	}
	
	UClass* Class = Cast<UClass>(StaticLoadObject(UClass::StaticClass(), 
		NULL, (const TCHAR*)mono_string_to_utf16(path)));
	if (Class == NULL) 
	{
		GLog->Log(ELogVerbosity::Error, TEXT("[World] Can't GetClass When SpwanActor."));
		return NULL;
	}

	return GWorld->SpawnActor(Class, trans);
}

static UMaterialInterface* UnrealEngine_Resource_LoadMaterial(MonoString* path)
{
	if (path == NULL)
	{
		GLog->Log(ELogVerbosity::Error, TEXT("[World] LoadMaterial But path is Null."));
		return NULL;
	}

	const TCHAR* p = (const TCHAR*)mono_string_to_utf16(path);
	UMaterialInterface* mat = Cast<UMaterialInterface>(
		StaticLoadObject(UMaterialInterface::StaticClass(), 
			NULL,p));
	if (mat == NULL) 
	{
		GLog->Logf(ELogVerbosity::Error, TEXT("[Resource] Can't LoadMaterial %s."),p);
		return NULL;
	}

	return mat;
}

static UTexture* UnrealEngine_Resource_LoadTexture(MonoString* path)
{
	if (path == NULL)
	{
		GLog->Log(ELogVerbosity::Error, TEXT("[World] LoadTexture But path is Null."));
		return NULL;
	}

	const TCHAR* p = (const TCHAR*)mono_string_to_utf16(path);
	UTexture* texture = Cast<UTexture>(
		StaticLoadObject(UTexture::StaticClass(),NULL, p));
	
	if (texture == NULL)
	{
		GLog->Logf(ELogVerbosity::Error, TEXT("[Resource] Can't LoadTexture %s."), p);
		return NULL;
	}

	return texture;
}

static void UnrealEngine_Resource_GC()
{
	if (GWorld.GetReference() == NULL)
	{
		GLog->Log(ELogVerbosity::Error, TEXT("[World] Can't GetWorld When GC."));
		return ;
	}
	GWorld->GetWorld()->ForceGarbageCollection(true);
}

static mono_bool UnrealEngine_UObject_GetIsRooted(UObject* object)
{
	return object->IsRooted();
}

static void UnrealEngine_UObject_SetIsRooted(UObject* object, mono_bool value)
{
	if (value != 0) 
	{
		object->AddToRoot();
	}
	else {
		object->RemoveFromRoot();
	}	
}



void UnrealAPI_Engine::RegisterAPI()
{
	mono_trace_set_log_handler(MonoLog, NULL);
	mono_trace_set_print_handler(MonoPrintf);
	mono_trace_set_printerr_handler(MonoPrintf);

	mono_add_internal_call("UnrealEngine.Log::Debug",
		reinterpret_cast<void*>(UnrealEngine_Log_Debug));
	mono_add_internal_call("UnrealEngine.Log::Warning", 
		reinterpret_cast<void*>(UnrealEngine_Log_Warning));
	mono_add_internal_call("UnrealEngine.Log::Error", 
		reinterpret_cast<void*>(UnrealEngine_Log_Error));

	mono_add_internal_call("UnrealEngine.World::GetCurrentLevel", 
		reinterpret_cast<void*>(UnrealEngine_World_GetCurrentLevel));
	mono_add_internal_call("UnrealEngine.World::LoadStreamingLevel",
		reinterpret_cast<void*>(UnrealEngine_World_LoadStreamingLevel));
	mono_add_internal_call("UnrealEngine.World::UnLoadStreamingLevel",
		reinterpret_cast<void*>(UnrealEngine_World_UnLoadStreamingLevel)); 
	mono_add_internal_call("UnrealEngine.World::_SpwanActor",
			reinterpret_cast<void*>(UnrealEngine_World_SpwanActor));

	mono_add_internal_call("UnrealEngine.Resource::_LoadMaterial",
		reinterpret_cast<void*>(UnrealEngine_Resource_LoadMaterial));
	mono_add_internal_call("UnrealEngine.Resource::_LoadTexture",
		reinterpret_cast<void*>(UnrealEngine_Resource_LoadTexture));
	mono_add_internal_call("UnrealEngine.Resource::_GC",
		reinterpret_cast<void*>(UnrealEngine_Resource_GC));

	mono_add_internal_call("UnrealEngine.UObject::_GetIsRooted",
		reinterpret_cast<void*>(UnrealEngine_UObject_GetIsRooted));
	mono_add_internal_call("UnrealEngine.UObject::_SetIsRooted",
		reinterpret_cast<void*>(UnrealEngine_UObject_SetIsRooted));

	

}


