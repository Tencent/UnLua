#pragma once
#include "BlueprintEditor.h"

class FUnLuaEditorToolbar : public TSharedFromThis<FUnLuaEditorToolbar>
{
public:
	FUnLuaEditorToolbar();
	virtual ~FUnLuaEditorToolbar() = default;

	TSharedRef<FUICommandList> GetCommandList() const
	{
		return CommandList;
	}

	virtual void Initialize();

	void CreateLuaTemplate_Executed();

	void CopyAsRelativePath_Executed() const;

	void BindToLua_Executed() const;

	void UnbindFromLua_Executed() const;

	FSlateIcon GetStatusImage() const;
protected:
	virtual void BindCommands();

	void BuildToolbar(FToolBarBuilder& ToolbarBuilder);

	TSharedRef<FExtender> GetExtender();

	const TSharedRef<FUICommandList> CommandList;

	UObject* ContextObject;

	TWeakPtr<IBlueprintEditor> BlueprintEditorPtr;
};
