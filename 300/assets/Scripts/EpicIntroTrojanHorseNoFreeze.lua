-- Variables for state
local s1Timer           = 2
local s1RoamVelocity    = Vec3.new()
local roamSpeed         = 2
local damage            = 20

local s2ChargeCount     = 0
local s2Charge          = 2

local s3SprintVelocity  = Vec3.new()
local sprintSpeed       = 15
local stareDirection    = Vec3.new()

local s4RestTimer       = 2
local s4RestTimerCount  = 0

-- Systems
local aiSys
local phySys
local gameStateSys

-- Other variables
local this
local target

local deathTimer = 1.25
local deathTimerCount

local spawnSoldierFrequency = 0.6
local spawnSoldierCounter = 0

local state

local audio_played = false

local isHit
local countHit
-- Trojan horse states
-- 1. ROAM. roam around and passively look for player (change to 2. when sees player)
-- 2. CHARGE. saw player, eyes glow red, play some charge up noise, delay about 3 seconds before charging to player (change to 3. when delay ends)
-- 3. SPRINT. charge toward last seen player position at high speed (change to 4. when collided with something)
-- 4. REST. stops for around 0.5 seconds before moving back to 1. (change to 1. when rest timer ends)

function Alive()
    -- print("Alive()")
    math.randomseed(os.time())

    this = Helper.GetScriptEntity(script_entity.id)
    if this == nil then
        print("Entity nil in Trojan Horse script!")
    end

    aiSys = systemManager:mAISystem();
    phySys = systemManager:mPhysicsSystem();
    gameStateSys = systemManager:mGameStateSystem();

    -- Initialise the state's variables
    state = "ROAM"
    -- this:GetAudio():SetPlay()
    s1Timer           = 2
    s1RoamVelocity    = Vec3.new()
    s2Timer           = 0
    s3SprintVelocity  = Vec3.new()

    deathTimerCount   = 0
    target = this:GetAISetting():GetTarget()
    isHit = false
    countHit = 0.0
end

local timer = 0
function Update()
    ChangeColorOnHit()
    -- If trojan horse epic intro is between state 4 and 6, return

    -- dt = FPSManager.GetDT()

    -- timer = timer - dt 

    -- if (timer < 0) then
    --     this:GetAudio():SetPlay()
    --     timer = 3
    -- end


    -- OTHER UPDATE CODES
    -- if systemManager:mInputActionSystem():GetButtonDown("Test2") then
    --     this:GetHealthbar().health = this:GetHealthbar().health - 100
    -- end

    -- STATE MACHINE
    if state == "ROAM" then         -- roam around and passively look for player (change to 2. when sees player)
        -- Roam around randomly
        s1Timer = s1Timer + FPSManager.GetDT()
        Helper.SetRealRotate(this, Vec3.new(0,Helper.DirectionToAngle(this, s1RoamVelocity),0))

        if s1Timer > 2 then
            s1Timer = 0
            MoveRandDir()
        end
        phySys:SetVelocity(this, s1RoamVelocity)

        -- Look for player here
        if aiSys:ConeOfSight(this, target, 70, 40) then
            -- this:GetAudio():SetPlay()
            CHARGEInit()
        end

    elseif state == "CHARGE" then   -- saw player, eyes glow red, play some charge up noise, delay about 3 seconds before charging to player (change to 3. when delay ends)
        -- Play animation for eyes glowing red        
        
        -- Constantly make him stare at player and stand still
        stareDirection = Helper.Vec3Minus(target:GetTransform().mTranslate, this:GetTransform().mTranslate)
        Helper.SetRealRotate(this, Vec3.new(0,Helper.DirectionToAngle(this, stareDirection),0))
        phySys:SetVelocity(this, Vec3.new())
        
        -- if audio_played == false then 
        --     this:GetAudio():SetPlay()
        -- end

        -- Count down 3 seconds
        s2ChargeCount = s2ChargeCount + FPSManager.GetDT()
        if s2ChargeCount > s2Charge then
            s2ChargeCount = 0
            SPRINTInit()

            s3SprintVelocity = Helper.Scale(Helper.Normalize(stareDirection), sprintSpeed)
            s3SprintVelocity.y = 0
            -- TODO End audio and animation
        end

    elseif state == "SPRINT" then   -- charge toward last seen player position at high speed (change to 4. when collided with something)
        Helper.SetRealRotate(this, Vec3.new(0,Helper.DirectionToAngle(this, stareDirection),0))
        if (_G.TrojanHorseEpicIntroState >= 5 and _G.TrojanHorseEpicIntroState <= 7) then return end
        -- Charge towards last seen player position at high speed
        phySys:SetVelocity(this, s3SprintVelocity);
        -- Stop and change state when collided with something
        -- This part is done in OnContactEnter

    elseif state == "REST" then     -- stops for some time before moving back to 1. (change to 1. when rest timer ends)
        Helper.SetRealRotate(this, Vec3.new(0,Helper.DirectionToAngle(this, stareDirection),0))
        s4RestTimerCount = s4RestTimerCount + FPSManager.GetDT()
        if s4RestTimerCount > s4RestTimer then
            s4RestTimerCount = 0
            ROAMInit()
        end
    elseif state == "DEATH" then
        -- spawning soldier
        local dt = FPSManager.GetDT()
        spawnSoldierCounter = spawnSoldierCounter + dt
        deathTimerCount = deathTimerCount + dt

        if spawnSoldierCounter > spawnSoldierFrequency then
            spawnSoldierCounter = 0
            SpawnSoldier()
        end
        if deathTimerCount > deathTimer then Die() end
        return
    end
    -- END STATE MACHINE

    -- Health logic
    if this:GetHealthbar().health <= 0 then
        StartDeath() 
        if _G.Level3_Monsters ~= nil then 
            if _G.Level3_Monsters == true then 
                _G.number_left_in_level_3 = _G.number_left_in_level_3 - 1
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
    if (state == "SPRINT") then
        local generalComponent = Entity:GetGeneral()
        local tagid = generalComponent.tagid
        if (tagid ~= 2 and tagid ~= 3 and tagid ~= 7) then -- "BULLET", "FLOOR", "GRAPH"
            -- When collide with anything other than those tag, stop and change state
            if Entity == target then CrashIntoPlayer() end

            state = "REST"
            RESTInit()
            return
        end
    elseif state == "ROAM" then
        MoveRandDir()
    end
end

function OnContactExit(Entity)

end


-- State initialise functions

function ROAMInit()
    --print("Start Roam")
    state = "ROAM"
    s1Timer = 2
    s1RoamVelocity = RandDirectionXZ()
    this:GetAudio():SetResume()
    this:GetMeshRenderer():SetMesh("Trojan_Movement", this)
end

function CHARGEInit()
    -- play animation
    this:GetMeshRenderer():SetMesh("Trojan_charging", this)

    --print("Start Charge")
    -- this:GetAudio():SetPause()

    -- [1/31] ML - changed to 3D system (for spatial cues) -> audio file is now attached w the enemy.
    this:GetAudio():SetPlay()
    -- gameStateSys:GetEntity("TrojanHorseChargeAudio"):GetAudio():SetPlay()
    state = "CHARGE"
    s2ChargeCount = 0
end

function SPRINTInit()
    --print("Start Sprint")
    state = "SPRINT"
    this:GetAudio():SetResume()
end

function RESTInit()
    --print("Start Rest")
    state = "REST"
    this:GetMeshRenderer():SetMesh("Trojan_Idle", this) -- change animation back

    -- this:GetAudio():SetPause()
    phySys:SetVelocity(this, Vec3.new())
end


-- Helper functions

function RandDirectionXZ()
    local v = Vec3.new()
    v.x = math.random()*2 -1
    v.y = 0
    v.z = math.random()*2 -1
    return v
end

function MoveRandDir()
    s1RoamVelocity = RandDirectionXZ()
    s1RoamVelocity = Helper.Normalize(s1RoamVelocity)
    Helper.SetRealRotate(this, Vec3.new(0,Helper.DirectionToAngle(this, s1RoamVelocity),0))
    s1RoamVelocity = Helper.Scale(s1RoamVelocity, roamSpeed)
end

-- this function is ran when health just reached 0
function StartDeath()
    -- Start death animation
    -- Start death sound
    state = "DEATH"
    this:GetMeshRenderer():SetMesh("Trojan_Death", this)
    gameStateSys:GetEntity("EnemyDeath"):GetAudio():SetPlay()
end

function Die()
    systemManager.ecs:SetDeleteEntity(this)
end

function SpawnSoldier()
    local position = Helper.Vec3Add(this:GetTransform().mTranslate, this:GetBoxCollider().mTranslateOffset)
    local velocity = Helper.Scale(RandDirectionXZ(), 15)
    velocity.y = 24
    
    local soldier = systemManager.ecs:NewEntityFromPrefab("TrojanSoldier", position)
    phySys:SetVelocity(soldier, velocity)
end

function CrashIntoPlayer()
    -- decrease player health
    target:GetHealthbar().health = target:GetHealthbar().health - damage

    -- make player fly back
    local velocity = Helper.Scale(Helper.Normalize(this:GetRigidBody().mVelocity), 100)
    velocity.y = 20
    phySys:SetVelocity(target, velocity)
end

function OnOtherTriggerEnter(Entity)
    if Entity:GetGeneral().tagid ~= 2 and Entity:GetGeneral().tagid ~= 9 and Entity:GetGeneral().tagid ~= 10 and Entity:GetGeneral().tagid ~= 11 and Entity:GetGeneral().tagid~= 12 then
        return
    end
    isHit = true
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