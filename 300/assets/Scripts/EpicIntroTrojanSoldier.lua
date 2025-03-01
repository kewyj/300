-- only for epic intro trojan soldier
-- Variables for state

-- Systems
local phySys
local gameStateSys

-- Other variables
local this
local target
local damage = 5

local deathTimer = 0.1
local deathTimerCount

local state

local aiSetting

local isHit
local countHit

-- Trojan horse states
-- 1. ROAM. roam around and passively look for player (change to 2. when sees player)
-- 2. CHARGE. saw player, eyes glow red, play some charge up noise, delay about 3 seconds before charging to player (change to 3. when delay ends)
-- 3. SPRINT. charge toward last seen player position at high speed (change to 4. when collided with something)
-- 4. REST. stops for around 0.5 seconds before moving back to 1. (change to 1. when rest timer ends)

function Alive()
    math.randomseed(os.time())

    this = Helper.GetScriptEntity(script_entity.id)
    if this == nil then print("Entity nil in script!") end

    phySys = systemManager:mPhysicsSystem();
    aiSetting = this:GetAISetting()
    gameStateSys = systemManager:mGameStateSystem();

    -- Initialise the state's variables
    state = ""

    deathTimerCount   = 0
    target = this:GetAISetting():GetTarget()
    
    isHit = false
    countHit = 0.0
end

function Update()
    ChangeColorOnHit()

    if state == "DEATH" then
        deathTimerCount = deathTimerCount + FPSManager.GetDT()
        if deathTimerCount > deathTimer then systemManager.ecs:SetDeleteEntity(this) end
        return
    end

    -- Attack portion is in oncontact
    -- Movement
    Helper.SetRealRotate(this, Vec3.new(0, Helper.DirectionToAngle(this, this:GetRigidBody().mVelocity), 0))
    
    if Helper.Vec3Len(this:GetRigidBody().mVelocity) < 0.1 then
        local upVec = Vec3.new()
        upVec.y = 8
        phySys:SetVelocity(this, upVec);
    end

    if this:GetHealthbar().health <= 0 then StartDeath() end


end

function ChangeColorOnHit()
    this = Helper.GetScriptEntity(script_entity.id)
    if isHit == true then
        this:GetMeshRenderer():SetColor(Vec4.new(0.05,0.05,0.05,1))
        if countHit < 0.1 then 
            countHit = countHit + FPSManager.GetDT()
        else 
            isHit = false
            countHit = 0
        end
    else
        this:GetMeshRenderer():SetColor(Vec4.new(1,1,1,1))
    end
end

function OnContactEnter(Entity)
    if state == "DEATH" then return end
    
    local tagID = Entity:GetGeneral().tagid
    local velocity = Helper.Normalize(Helper.Vec3Minus(aiSetting:GetTarget():GetTransform().mTranslate, this:GetTransform().mTranslate))
    velocity = Helper.Scale(velocity, 3)
    velocity.y = 8
    if tagID == 3 then -- "FLOOR"
        -- Movement
    elseif Entity == aiSetting:GetTarget() then
        -- Attack
        velocity.x = -velocity.x * 5
        velocity.y = velocity.y * 3
        velocity.z = -velocity.z * 5

        -- decrease player health
        target:GetHealthbar().health = target:GetHealthbar().health - damage
    end
    phySys:SetVelocity(this, velocity);
    this:GetAudio():SetPlay()
end


-- this function is ran when health just reached 0
function StartDeath()
    -- Start death animation
    -- Start death sound
    state = "DEATH"
    gameStateSys:GetEntity("EnemyDeath"):GetAudio():SetPlay()
end










function Dead()

end

function OnTriggerEnter(Entity)
end

function OnTriggerExit(Entity)
end

function OnOtherTriggerEnter(Entity)
    if Entity:GetGeneral().tagid ~= 2 and Entity:GetGeneral().tagid ~= 9 and Entity:GetGeneral().tagid ~= 10 and Entity:GetGeneral().tagid ~= 11 and Entity:GetGeneral().tagid~= 12 then
        return
    end
    isHit = true
end

function OnOtherTriggerExit(Entity)
end

function OnContactExit(Entity)

end
