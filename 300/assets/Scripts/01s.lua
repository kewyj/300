local destroyHeight = 60 -- height to destroy the object
local tpcolor = Vec4.new(1, 1, 1, 0)

local target = Vec3.new(0,0,0)

local direction = Vec3.new()

local yjisgay = 0

function Alive()
    -- ent = Helper.GetScriptEntity(script_entity.id)
    -- transform = ent:GetTransform()
    -- transform.mRotate.y = math.random(0,300)

    math.randomseed(os.time())
    yjisgay = math.random(2,5)

end

function Update()



    gameStateSys = systemManager:mGameStateSystem()
    ent = Helper.GetScriptEntity(script_entity.id)
    transform = ent:GetTransform()




    transform.mRotate.y = transform.mRotate.y+yjisgay

    if( transform.mRotate.y >  360)then
        transform.mRotate.y = 0
    end

    player = gameStateSys:GetEntityByScene("Camera" , "testSerialization")
    playerTrans = player:GetTransform()


    transform.mTranslate.y = transform.mTranslate.y+0.5

    direction.x = transform.mTranslate.x - playerTrans.mTranslate.x
    direction.y = transform.mTranslate.y - playerTrans.mTranslate.y
    direction.z = transform.mTranslate.z - playerTrans.mTranslate.z


    -- tpcolor.y = tpcolor.y-0.01
     mesi = ent:GetMeshRenderer()

    --  transform.mRotate.y = Helper.DirectionToAngle(this, direction)
    -- mesi:SetColor(tpcolor)
    
    if(transform.mTranslate.y > destroyHeight)then
        systemManager.ecs:SetDeleteEntity(ent)
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


