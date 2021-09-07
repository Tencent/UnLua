

#if ENABLE_PERFORMANCE_TEST == 1

    #include "Tests/UnLuaPerformanceTestProxy.h"
    #include "Misc/DateTime.h"
	#include "Misc/FileHelper.h"
	#include "UnLuaEx.h"

    void RunPerformanceTest(UWorld *World)
    {
        if (!World)
        {
            return;
        }
        static AActor *PerformanceTestProxy = World->SpawnActor(AUnLuaPerformanceTestProxy::StaticClass());
    }

	bool LogPerformanceData(const FString &Message)
	{
	    FString ProfilingFilePath = FString::Printf(TEXT("%sProfiling/UnLua-Performance-%s.csv"), *FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()), *FDateTime::Now().ToString());
	    bool bSuccess = FFileHelper::SaveStringToFile(Message, *ProfilingFilePath);
	    return bSuccess;
	}

    EXPORT_FUNCTION(void, RunPerformanceTest, UWorld *)
	EXPORT_FUNCTION(bool, LogPerformanceData, const FString&)
	EXPORT_FUNCTION_EX(Seconds, double, FPlatformTime::Seconds)

#endif