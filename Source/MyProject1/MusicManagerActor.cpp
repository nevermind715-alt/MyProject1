#include "MusicManagerActor.h"

AMusicManagerActor::AMusicManagerActor()
{
	// このアクタは毎フレームのTick処理が不要なため、オフにして処理負荷を下げます
	PrimaryActorTick.bCanEverTick = false;
}