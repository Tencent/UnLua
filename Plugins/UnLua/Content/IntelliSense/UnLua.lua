---@class UnLua
local UnLua = {}

---Similar to "package.path" for lua module searching with UnLua loader. Used by FLuaEnv::LoadFromFileSystem.
UnLua.PackagePath = "Content/Script/?.lua;Plugins/UnLua/Content/Script/?.lua"

---Whether or not enable FText support at lua runtime which will no longer be treated as a string.
UnLua.FTextEnabled = false

---Define a lua class for binding.
---@param SuperClass string @[opt]
function UnLua.Class(SuperClass)
end

---Hot reload lua modules
---@param ModuleNames table @[opt]Specify a list of module names that need hot reloading. If this parameter is not passed, it will be obtained by default according to the file modification timestamp
function UnLua.HotReload(ModuleNames)
end

---Add or find a manual reference to specified UObject. This prevents the object from UE GC.
---@param Object UObject
---@return userdata @Proxy object of the reference. The reference of UObject will be removed when proxy object deleted by lua GC.
function UnLua.Ref(Object)
end

---Force remove all manual reference to specified UObject.
---@param Object UObject
function UnLua.Unref(Object)
end

_G.UnLua = UnLua

---@class TArray<TElement>
local TArray = {}

---Get the number of items in an array
---@return integer
function TArray:Length()
end

---Get the number of items in an array
---@return integer
function TArray:Num()
end

---Add item to array
---@param NewItem TElement @The item to look for
---@return integer @The index of the newly added item
function TArray:Add(NewItem)
end

---Add item to array (unique)
---@param NewItem TElement @The item to add to the array
---@return integer @The index of the newly added item, or INDEX_NONE if the item is already present in the array
function TArray:AddUnique(NewItem)
end

---Finds the index of the first instance of the item within the array
---@param ItemToFind TElement @The item to look for
---@return integer @The index the item was found at, or -1 if not found
function TArray:Find(ItemToFind)
end

---Insert item at the given index into the array.
---@param NewItem TElement @The item to insert into the array
---@param Index integer @The index at which to insert the item into the array
function TArray:Insert(NewItem, Index)
end

---Remove item at the given index from the array.
---@param IndexToRemove integer @The index into the array to remove from
function TArray:Remove(IndexToRemove)
end

---Remove all instances of item from array.
---@param Item TElement @The item to remove from the array
---@return boolean @True if one or more items were removed
function TArray:RemoveItem(Item)
end

---Clear the array, removes all content
function TArray:Clear()
end

---Reserve space for N elements
---@param Size integer @Size
---@return bool @whether the operation succeed
function TArray:Reserve(Size)
end

---Resize Array to specified size. 
---@param Size integer @The new size of the array
function TArray:Resize(Size)
end

---Get address of the i'th element
---@param Index integer @the index
---return lightuserdata @the address of the i'th element
function TArray:GetData(Index)
end

---Given an array and an index, returns a copy of the item found at that index
---@param Index integer @The index in the array to get an item from
---@return any @A copy of the item stored at the index
function TArray:Get(Index)
end

---Given an array and an index, returns a reference of the item found at that index
---@param Index integer @The index in the array to get an item from
---@return TElement @A reference of the item stored at the index
function TArray:GetRef(Index)
end

---Given an array and an index, assigns the item to that array element
---@param Index integer @The index to assign the item to
---@param Item TElement @The item to assign to the index of the array
function TArray:Set(Index, Item)
end

---Swaps the elements at the specified positions
---If the specified positions are equal, invoking this method leaves the array unchanged
---@param FirstIndex integer @The index of one element to be swapped
---@param SecondIndex integer @The index of the other element to be swapped
function TArray:Swap(FirstIndex, SecondIndex)
end

---Shuffle (randomize) the elements
function TArray:Shuffle()
end

---Get the last valid index
---@return integer @The last valid index of the array
function TArray:LastIndex()
end

---Tests if IndexToTest is valid, i.e. greater than or equal to zero, and less than the number of elements in array.
---@param IndexToTest integer @The Index, that we want to test for being valid
---@return boolean @True if the Index is Valid, i.e. greater than or equal to zero, and less than the number of elements in array.
function TArray:IsValidIndex(IndexToTest)
end

---Returns true if the array contains the given item
---@param ItemToFind any @The item to look for
---@return boolean @True if the item was found within the array
function TArray:Contains(ItemToFind)
end

---Append an array to another array
---@param OtherArray TArray @The array to add
function TArray:Append(OtherArray)
end

---Get a lua table copy of this array.
---@return table
function TArray:ToTable()
end

---@type fun(ElementType:any):TArray
UE.TArray = TArray

---@class TMap<TKey,TValue>
local TMap = {}

---Determines the number of entries in map
---@return integer
function TMap:Length()
end

---Determines the number of entries in map
---@return integer
function TMap:Num()
end

---Adds a key and value to the map. If something already uses the provided key it will be overwritten with the new value.
---After calling Key is guaranteed to be associated with Value until a subsequent mutation of the Map.
---@param Key TKey @The key that will be used to look the value up
---@param Value TValue @The value to be retrieved later
function TMap:Add(Key, Value)
end

---Removes a key and its associated value from the map.
---@param Key any @The key that will be used to look the value up
---@return boolean @True if an item was removed (False indicates nothing in the map uses the provided key)
function TMap:Remove(Key)
end

---Finds the value associated with the provided Key
---@param Key any @The key that will be used to look the value up
---@return any @The value associated with the key
function TMap:Find(Key)
end

---Finds the value associated with the provided Key
---@param Key any @The key that will be used to look the value up
---@return any @The value associated with the key
function TMap:FindRef(Key)
end

---Clears all entries, resetting it to empty
function TMap:Clear()
end

---Outputs an array of all keys present in the map
---@return TArray @All keys present in the map
function TMap:Keys()
end

---Outputs an array of all values present in the map
---@return TArray @All values present in the map
function TMap:Values()
end

---Get a lua table copy of this map.
---@return table
function TMap:ToTable()
end

---@type fun(KeyType:any,ValueType:any):TMap
UE.TMap = TMap

---@class TSet<TElement>
local TSet = {}

---Get the number of items in set.
---@return integer
function TSet:Length()
end

---Get the number of items in set.
---@return integer
function TSet:Num()
end

---Adds item to set
---@param NewItem TElement @The item to add to the set
function TSet:Add(NewItem)
end

---Remove item from set
---@param Item TElement @The item to remove from the set
---@return boolean @True if an item was removed (False indicates no equivalent item was present)
function TSet:Remove(Item)
end

---Returns true if the set contains the given item.
---@param ItemToFind any @The item to look for
---@return boolean @True if the item was found within the set
function TSet:Contains(ItemToFind)
end

---Clear the set, removes all content.
function TSet:Clear()
end

---Outputs an Array containing copies of the entries of a Set.
---@return TArray @Array
function TSet:ToArray()
end

---Get a lua table copy of this set.
---@return table
function TSet:ToTable()
end

---@type fun(ElementType:any):TSet
UE.TSet = TSet

---@class UClass
local UClass = {}

---Load a class object
---@param Path string @path to the class
---@return UClass
function UClass.Load(Path)
end

---Test whether this class is a child of another class
---@param TargetClass UClass @target class
---@return boolean @true if this object is of the specified type.
function UClass:IsChildOf(TargetClass)
end

---Get default object of a class.
---@return UObject @class default obejct
function UClass:GetDefaultObject()
end

UE.UClass = UClass

---@class MulticastDelegate
local MulticastDelegate = {}

---Add a callback to the delegate
---@param Object UObject @receiver object
---@param Callback function @callback
function MulticastDelegate:Add(Object, Function)
end

---Remove a callback from the delegate
---@param Object UObject @receiver object
---@param Callback function @callback
function MulticastDelegate:Remove(Object, Function)
end

---Clear all callbacks for the delegate
function MulticastDelegate:Clear()
end

---Call all callbacks bound to the delegate
function MulticastDelegate:Broadcast()
end

---Detect if any callback bound to this delegate
---@return boolean
function MulticastDelegate:IsBound()
end
