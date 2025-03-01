_G.PreObjectivesCounter = 0;
local objective1
local objective2
local objective3
local initonce
function Alive()
    gameStateSys = systemManager:mGameStateSystem()
    initonce = false
end

function Update()
    --set >= 4 for now as there are 4 'A' enemies in the scene rn
    -- print("OBJECTIVE COUNTER: " , _G.PreObjectivesCounter)
    --if _G.PreObjectivesCounter >= 4 then
    if(_G.completedEpicTH == true and _G.completedEpicTS == true and _G.completedEpicILY == true) 
    then
        if initonce==false 
        then
            controllerL2 = gameStateSys:GetEntity("DialogueController")
            controllerL2Scripts = controllerL2:GetScripts()
            controllerL2Script = controllerL2Scripts:GetScript("../assets/Scripts/DialogueControllerLevel1.lua")
            
            ent = gameStateSys:GetEntityByScene("ObjectiveIndicatorUI" , "UI")
            uirend = ent:GetUIrenderer()
            uirend:SetTexture("0_3_Installed_Text")
            uirend.mColor.w = 1.0
            _G.objectiveTimer = 0.0
            _G.ObjectiveIndicatorUI_Texture = "default"

            if controllerL2Script ~= nil then
                controllerL2Script:RunFunction("FinishedCutscenes")
            end
            initonce = true
        end

        gameStateSys = systemManager:mGameStateSystem();
        testScriptEntity = gameStateSys:GetEntity("Controller")
        TestScripts = testScriptEntity:GetScripts()
        testScript = TestScripts:GetScript("../assets/Scripts/ObjectivesController.lua")
        objCount = testScript:ReturnValueInt("GetCountObj")

        --spawn the objectives portals
        -- print("SPAWN PORTALS")
        objective1 = gameStateSys:GetEntity("Objectives1")
        objective2 = gameStateSys:GetEntity("Objectives2")
        objective3 = gameStateSys:GetEntity("Objectives3")

        local o1offset = Vec3.new()
        local o2offset = Vec3.new()
        local o3offset =Vec3.new()
        -- use the counter to raise the objective platforms
        -- updated 9-3-2024 new objectives positions
        if testScript ~= nil then
            if objCount == 3 then
                -- spawn get to objective
                o1offset.x = objective1:GetTransform().mTranslate.x
                o1offset.y = -6.555
                o1offset.z = objective1:GetTransform().mTranslate.z
                Helper.SetTranslate(objective1,o1offset)
            end
            
            if objCount == 2 then
                o2offset.x = objective2:GetTransform().mTranslate.x
                o2offset.y = -11
                o2offset.z = objective2:GetTransform().mTranslate.z
                Helper.SetTranslate(objective2,o2offset)
            end

            if objCount == 1 then
                o3offset.x = objective3:GetTransform().mTranslate.x
                o3offset.y = -6.555
                o3offset.z = objective3:GetTransform().mTranslate.z
                Helper.SetTranslate(objective3,o3offset)
            end
        end
    end
end

function Dead()

end

function OnTriggerEnter(Entity)
    
end

function OnTriggerExit(Entity)
    
end

function OnContactEnter(Entity)

end

function OnContactExit(Entity)

end


