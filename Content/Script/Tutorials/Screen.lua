local M = {}

local PrintString = UE4.UKismetSystemLibrary.PrintString

function M.Print(text, color, duration)
    color = color or UE4.FLinearColor(1, 1, 1, 1)
    duration = duration or 100
    PrintString(nil, text, true, false, color, duration)
end

return M