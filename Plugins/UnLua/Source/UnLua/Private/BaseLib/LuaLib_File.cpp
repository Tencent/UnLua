#include "UnLuaEx.h"
#include "Containers/LuaArray.h"
#include "LuaCore.h"
#include "lauxlib.h"
#include "luaconf.h"
#include "HAL/FileManager.h"



class UE4File
{
public:
	UE4File()
	{
		this->bWirte = false;
		this->Flags = 0x00;
		this->FilePath = TEXT("");
	}

	~UE4File()
	{
		FILE.Reset();
	}
public:

	bool Open(const FString&InFilePath,const FString& Mode)
	{
		bool Ret = false;
		
		if (this->HelperCheckFileMode(Mode, this->Flags, this->bWirte))
		{
			FILE.Reset();
			if (this->bWirte)
			{
				FArchive* FilePtr = IFileManager::Get().CreateFileWriter(*InFilePath, this->Flags);
				if (nullptr != FilePtr)
				{
					FILE = MakeShareable(FilePtr);
					Ret = true;
				}

			}
			else
			{
				FArchive* FilePtr = IFileManager::Get().CreateFileReader(*InFilePath, this->Flags);
				if (nullptr != FilePtr)
				{
					FILE = MakeShareable(FilePtr);
					Ret = true;
				}

			}
		}
		else
		{
			;//do nothing
		}

		return Ret;
	}

	void Close()
	{
		FILE.Reset();
	}
	void  Seek(const FString& Mode, int32 Offset)
	{
		if (FILE.IsValid())
		{
			static const FString modenames[] = { TEXT("set"), TEXT("cur"), TEXT("end")};
			if (Mode.Equals(modenames[0]))
			{
				FILE->Seek(Offset);
			}
			else if (Mode.Equals(modenames[1]))
			{
				FILE->Seek(FILE->Tell() + Offset);
			}
			else if (Mode.Equals(modenames[2]))
			{
				int64 Length = FILE->TotalSize();
				int64 RealOffset = Length - Offset;
				FILE->Seek(RealOffset);
			}
		}

	}

	int32 TotalSize()
	{
		if (FILE.IsValid())
		{
			return FILE->TotalSize();
		}
		return 0;
	}

	bool IsValid()
	{
		return FILE.IsValid();
	}


	void Flush()
	{
		if (FILE.IsValid())
		{
			return FILE->Flush();
		}
	}


	TSharedPtr <FArchive> GetFArchive()
	{
		return FILE;
	}


	bool IsReadable()
	{
		return (this->FILE.IsValid() && (false == this->bWirte || this->Flags & FILEWRITE_AllowRead));
	}


	bool IsWriteable()
	{
		return (this->FILE.IsValid() && (true == this->bWirte || this->Flags & FILEREAD_AllowWrite));
	}


	//////////FUNCTIONS FOR LUA LIB//////////////////////
		/**
	 *
	 @param Size is Size is Zero Means Read All From Current Offset,else means read SizeBytes from current Offset
	 */
	FORCEINLINE  int ReadSize(TArray<uint8>& Buffer, uint32 Size = 0)
	{
		TArray<uint8> DataBuffer;
		static const int64 DataBufferSize = 10 * 1024;
		//when file is valid and is read mode or FILEWRITE_AllowRead flag is set
		if (this->IsReadable())
		{
			DataBuffer.AddUninitialized(DataBufferSize);//10K
			//get file size
			int64 FileSize = FILE->TotalSize();
			//get current offset
			int64 CurPos = FILE->Tell();
			//compute readable target size(size left to be read from current offset)
			int64 TargetSize = FileSize - CurPos;
			int64 ReadSize = 0;
			if (0 < Size)
			{
				TargetSize = FMath::Min(TargetSize, (int64)Size);
			}
			while (TargetSize)
			{
				CurPos = FILE->Tell();
				FILE->Serialize(DataBuffer.GetData(), FMath::Min(TargetSize, DataBufferSize));
				ReadSize = FILE->Tell() - CurPos;
				TargetSize -= ReadSize;
				Buffer.Append(DataBuffer.GetData(), ReadSize);
			}
		}
		return Buffer.Num();
	}

	//ReadLine
	FORCEINLINE void ReadLine(lua_State *L, bool bWithNewLineCh = false)
	{
		if (FILE->AtEnd())
		{
			lua_pushnil(L);
		}
		else
		{
			TArray<uint8> Buffer;
			uint8 Ch;
			while (!FILE->AtEnd())
			{
				FILE->SerializeBits(&Ch, sizeof(uint8) * 8);
				if (Ch == '\n')
				{
					if (bWithNewLineCh)
					{
						Buffer.Add(Ch);
					}
					break;
				}
				Buffer.Add(Ch);
			}
			lua_pushlstring(L, (const char*)Buffer.GetData(), Buffer.Num());
		}
	}


	FORCEINLINE  void ReadNumber(lua_State *L)
	{
		if (FILE->AtEnd())
		{
			lua_pushnumber(L, 0);
		}
		else
		{
			lua_Number Number = 0;
			FILE->SerializeBits(&Number, sizeof(lua_Number) * 8);
			lua_Integer IntegerNumber = (lua_Integer)floor((double)Number);
			if (Number - IntegerNumber > 0)
			{
				lua_pushnumber(L, Number);
			}
			else
			{
				lua_pushinteger(L, IntegerNumber);
			}
		}
	}


	FORCEINLINE void ReadBytes(lua_State* L, int32 TryReadNumber)
	{
		if (0 >= TryReadNumber)
		{
			//if current is the end of file
			if (FILE->AtEnd())
			{
				lua_pushnil(L);
			}
			else
			{//return an empty lua_string
				lua_pushliteral(L, "");
			}
		}
		else
		{
			//if current is the end of file
			if (FILE->AtEnd())
			{
				lua_pushnil(L);
			}
			else
			{
				//try read number of bytes (string buffer)
				TArray<uint8> Buffer;
				int32 ReadNumber = this->ReadSize(Buffer, TryReadNumber);
				if (0 >= ReadNumber)
				{
					lua_pushnil(L);
				}
				else
				{
					lua_pushlstring(L, (const char*)Buffer.GetData(), ReadNumber);
				}

			}
		}
	}
private:
	bool HelperCheckFileMode(const FString& Mode, uint32& OutFlags, bool& OutIsWrite)
	{
		bool Ret = false;
		//skip 'b' flag default is binary mode@TODO check me :see file history
		static const FString OpenModeNames[] = 
		{ 
			TEXT("r"), TEXT("w"), TEXT("r+"),
			TEXT("w+") ,TEXT("a") ,TEXT("a+"),
			TEXT("rb"),TEXT("wb"),TEXT("ab"),
			TEXT("rb+"),TEXT("wb+"),TEXT("ab+")
		};

#ifndef ARRAYSIZE
#define ARRAYSIZE(array) (sizeof(array) / sizeof(array[0]))
#endif
		uint32 index = 0;
		uint32 size = ARRAYSIZE(OpenModeNames);
		for (; index < size; index++)
		{
			if (Mode.Equals(OpenModeNames[index], ESearchCase::IgnoreCase))
			{
				Ret = true;
				break;
			}
		}

		if (index < size)
		{
			int32 FindIndex = 0;
			if (Mode.FindChar(TEXT('r'), FindIndex))
			{
				OutIsWrite = false;
				OutFlags |= FILEREAD_None;
				if (Mode.FindChar(TEXT('+'), FindIndex))
				{
					OutFlags |= FILEREAD_AllowWrite;
				}
			}
			else if (Mode.FindChar(TEXT('w'), FindIndex))
			{
				OutIsWrite = true;
				OutFlags |= FILEWRITE_None;
				if (Mode.FindChar(TEXT('+'), FindIndex))
				{
					OutFlags |= FILEWRITE_AllowRead;
				}
			}
			else if (Mode.FindChar(TEXT('a'), FindIndex))
			{
				OutIsWrite = true;
				OutFlags |= FILEWRITE_Append;
				if (Mode.FindChar(TEXT('+'), FindIndex))
				{
					OutFlags |= FILEWRITE_AllowRead;
				}
			}
		}
		return Ret;
	}

	TSharedPtr <FArchive> FILE;
	bool bWirte;
	uint32 Flags;
	FString FilePath;
};


static int32 UE4File_ReadFile(lua_State *L)
{
	int32 arg = 2;
	int32 first = arg;
	int32 nargs = lua_gettop(L) - 1;
	UE4File *File = (UE4File*)lua_touserdata(L, 1);
	if (!File || !File->IsValid() || !File->IsReadable())
	{
		return 0;
	}

	TSharedPtr<FArchive> fileArchive = File->GetFArchive();
	if (nargs < 1)
	{
		//read line
		TArray<uint8> Buffer;
		uint8 Ch;
		while (!fileArchive->AtEnd())
		{
			fileArchive->SerializeBits(&Ch, sizeof(uint8) * 8);
			if (Ch == '\n')
			{
				break;
			}
			Buffer.Add(Ch);
		}
		lua_pushlstring(L, (const char*)Buffer.GetData(), Buffer.Num());
		return 1;
	}
	else
	{
		for (; nargs--; arg++)
		{
			if (lua_type(L, arg) == LUA_TNUMBER)
			{
				int32 TryReadNumber = (int32)lua_tointeger(L, arg);
				File->ReadBytes(L, TryReadNumber);
			}
			else
			{
				const char *p = luaL_checkstring(L, arg);
				if (*p == '*') p++;  /* skip optional '*' (for compatibility) */
				switch (*p)
				{
				case 'n':
				{
					File->ReadNumber(L);
					break;
				}
				case 'l':
				{
					File->ReadLine(L,false);
					break;
				}
				case 'L':
				{
					File->ReadLine(L,true);
					break;
				}
				case 'a':
				{
					TArray<uint8> Buffer;
					int32 ReadNumber = File->ReadSize(Buffer);
					if (0 >= ReadNumber)
					{
						lua_pushliteral(L, "");
					}
					else
					{
						lua_pushlstring(L, (const char*)Buffer.GetData(), ReadNumber);
					}
					break;
				}
				default:
				{
					first += 1;
					break;
				}

				
				}
			}

		}
	}
	return arg - first;
}

static int32 UE4File_WriteFile(lua_State *L)
{
	int32 arg = 2;
	int32 nargs = lua_gettop(L) - 1;
	int32 status = 1;
	int32 bytesWrite = 0;
	UE4File *File = (UE4File*)lua_touserdata(L, 1);
	if (!File || !File->IsValid() || !File->IsWriteable())
	{
		return 0;
	}

	if (nargs < 1)
	{
		return 0;
	}
	TSharedPtr<FArchive> fileArchive = File->GetFArchive();
	for (; nargs--; arg++)
	{
		if (lua_type(L, arg) == LUA_TNUMBER)
		{
			if (lua_isinteger(L, arg))
			{
				lua_Number luaValue = (lua_Number)lua_tointeger(L, arg);
				fileArchive->SerializeBits(&luaValue, sizeof(lua_Number) * 8);
			}
			else
			{
				lua_Number luaValue = (lua_Number)lua_tonumber(L, arg);
				fileArchive->SerializeBits(&luaValue, sizeof(lua_Number) * 8);
			}
		}
		else
		{
			size_t t = 0;
			const char* str = luaL_checklstring(L, arg, &t);
			fileArchive->SerializeBits((void*)str, t * sizeof(char) * 8);
		}
	}

	lua_pushinteger(L, bytesWrite);
	return 1;
}


static int32 UE4File_Delete(lua_State *L)
{
	int32 NumParams = lua_gettop(L);
	if (NumParams != 1)
	{
		UE_LOG(LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
		return 0;
	}

	UE4File *File = (UE4File*)lua_touserdata(L, 1);
	if (!File)
	{
		return 0;
	}
	File->~UE4File();
	return 0;
}

static const luaL_Reg UE4FileLib[] =
{
	{"Read", UE4File_ReadFile },
	{"Write",UE4File_WriteFile},
	{"__gc",UE4File_Delete},
	{ nullptr, nullptr }
};


BEGIN_EXPORT_NAMED_CLASS(File,UE4File)
ADD_LIB(UE4FileLib)
ADD_FUNCTION(Open)
ADD_FUNCTION(Close)
ADD_FUNCTION(Seek)
ADD_FUNCTION(TotalSize)
ADD_FUNCTION(Flush)
ADD_FUNCTION(IsValid)
END_EXPORT_CLASS()
IMPLEMENT_EXPORTED_CLASS(File)
