


function Alive()
    gameStateSys = systemManager:mGameStateSystem()
    graphicssys = systemManager:mGraphicsSystem()

    graphicssys:HideCursor(false)
    print("MAIN MENU, CURSOR SHOWN")
end

function Update()
    -- Get entity attached to script
    entity = Helper.GetScriptEntity(script_entity.id)
    if entity == nil then
        print("Entity nil in script!")
    end

    gameStateSys = systemManager:mGameStateSystem();
    inputSys = systemManager:mInputActionSystem();

    if (inputSys:GetKeyDown(257)) then -- Press ENTER
        gameStateSys:ChangeGameState("Test")
    end
    
    -- if (systemManager:mInputActionSystem():GetKeyDown(67)) then -- Press C
    --     systemManager:mGraphicsSystem():HideCursor(false)
    -- end
end

function Dead()
    graphicssys:HideCursor(true)
    print("MAIN MENU, CURSOR HIDDEN")
end

function OnTriggerEnter(Entity)
    
end

function OnTrigger(Entity)
    
end

function OnTriggerExit(Entity)
    
end

function OnContactEnter(Entity)

end

function OnContactExit(Entity)

end
