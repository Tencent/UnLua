#pragma once
#include "BlueprintEditor.h"

class FUnLuaEditorToolbar
{
public:
	virtual ~FUnLuaEditorToolbar() = default;
	FUnLuaEditorToolbar();

	TSharedRef<FUICommandList> GetCommandList() const
	{
		return CommandList;
	}

	virtual void Initialize();

	void CreateLuaTemplate_Executed();

	void CopyAsRelativePath_Executed() const;

	void BindToLua_Executed() const;

	void UnbindFromLua_Executed() const;

protected:
	virtual void BindCommands();

	void BuildToolbar(FToolBarBuilder& ToolbarBuilder);

	TSharedRef<FExtender> GetExtender();

	const TSharedRef<FUICommandList> CommandList;

	UObject* ContextObject;
};
