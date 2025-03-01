local vec = Vec3.new()

local aiSys
local phySys
local gameStateSys
local this

local bobbleAngle
local bobbleFrequency
local bobbleIntensity

local targetPos
local thisPos

local AttackSpeed = 1.75
local AttackTimer = 0
local AttackAnimation = 2.375
local ShotSpeed
local Attacked = false -- This run once every attack 

local inLineOfSight

local deathTimer = 1.05
local deathTimerCount

local state

local this
local isHit
local countHit
function Alive()
    this = Helper.GetScriptEntity(script_entity.id)
    if this == nil then
        print("Entity nil in script!")
    end
    aiSys = systemManager:mAISystem();
    phySys = systemManager:mPhysicsSystem();
    gameStateSys = systemManager:mGameStateSystem();
    this:GetMeshRenderer():SetMesh("ILY_Idle", this) -- Change back to idle animation 
    
    bobbleAngle = 0
    bobbleFrequency = 1
    bobbleIntensity = 0.5

    ShotSpeed = 10

    state = ""
    deathTimerCount = 0

    -- Change pathfinding graph to match that of the gamestate
    if (gameStateSys:GetCurrentGameState().mName == "Test") then
        this:GetAISetting().mGraphDataName = "FlyingPath"
    elseif (gameStateSys:GetCurrentGameState().mName == "Test2") then
        this:GetAISetting().mGraphDataName = "FlyingPath2"
    elseif (gameStateSys:GetCurrentGameState().mName == "Test3") then
        this:GetAISetting().mGraphDataName = "FlyingPath3"
    end
        isHit = false
    countHit = 0.0
end

function Update()
    if _G.FreezePlayerControl == true then
        phySys:SetVelocity(this, Vec3.new(0,0,0))
        return
    end
    ChangeColorOnHit()
    targetPos = this:GetAISetting():GetTarget():GetTransform().mTranslate
    thisPos = this:GetTransform().mTranslate
    inLineOfSight = aiSys:LineOfSight(this, this:GetAISetting():GetTarget())

    if state~="DEATH" then
        if inLineOfSight and not (state == "ATTACK") then
            -- if (state == "ATTACK")
            --print(state)
            --print("Attack init")
            ATTACKinit()
        elseif not inLineOfSight and state ~= "IDLE" then
            if AttackTimer > AttackAnimation then -- If attack animation ended already
                --print()
                IDLEinit()
            end
        end
    end

    if state == "DEATH" then
        deathTimerCount = deathTimerCount + FPSManager.GetDT()
        if deathTimerCount > deathTimer then systemManager.ecs:SetDeleteEntity(this) end
        return
    elseif state == "ATTACK" then
        AttackTimer = AttackTimer + FPSManager.GetDT()
        if (not Attacked) and AttackTimer > AttackSpeed then
            --print(Attacked)
            Shoot()
            Attacked = true
        elseif AttackTimer > AttackAnimation then -- Attack animation ended, reset attack
            --print("Reset attack")
            AttackTimer = 0
            Attacked = false
            this:GetAnimator():SetFrame(0)
        end
    end

    -- if systemManager:mInputActionSystem():GetButtonDown("Test3") then
    --     this:GetHealthbar().health = this:GetHealthbar().health - 10
    -- end

    
    
    -- Health logic
    if this:GetHealthbar().health <= 0 then 
        StartDeath() 
        if _G.Level3_Monsters ~= nil then 
            if _G.Level3_Monsters == true then 
                _G.number_left_in_level_3 = _G.number_left_in_level_3 - 1
                print("UPDATE ENEMIES LEFT: " , _G.number_left_in_level_3)
            end
        end
    end

    ILOVEYOUMovement()


end

function Dead()

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


function ILOVEYOUMovement() 
    --#region Movement
    vec = aiSys:GetDirection(this)
    if inLineOfSight then
        Helper.SetRealRotate(this, Vec3.new(0, Helper.DirectionToAngle(this, vec), 0))
    else
        local dir = Vec3.new()
        dir = Helper.Normalize(Helper.Vec3Minus(targetPos, thisPos))
        Helper.SetRealRotate(this, Vec3.new(0, Helper.DirectionToAngle(this, dir), 0))
    end

    Helper.Scale(vec, 3)
-- when horizontal distance reached, don't move by getdirection
    if inLineOfSight then
        -- Add bobbling here
        
        --
        
        -- If horizontal length is reached, don't move horizontally
        local x = targetPos.x - thisPos.x
        local z = targetPos.z - thisPos.z
        local distDiff = Helper.Vec2Len(x,z) - this:GetAISetting().mStayAway
        if -2 < distDiff and distDiff < 2 then
            vec.x = 0
            vec.z = 0
            bobbleAngle = bobbleAngle + bobbleFrequency
            if bobbleAngle > 360 then bobbleAngle = 0 end
            vec.y = math.cos(bobbleAngle/180*math.pi) * bobbleIntensity
        else 
            bobbleAngle = bobbleAngle + bobbleFrequency
            if bobbleAngle > 360 then bobbleAngle = 0 end
            vec.y = vec.y + math.cos(bobbleAngle/180*math.pi) * bobbleIntensity
        end
    end
    --
    
    phySys:SetVelocity(this, vec);
    --#endregion
end


function ILOVEYOUAttack()
    if not inLineOfSight then return end -- No line of sight, don't attack
    this:GetMeshRenderer():SetMesh("ILY_attack", this) -- Change back to idle animation 
    AttackTimer = AttackTimer + FPSManager.GetDT()
    if (not Attacked) and AttackTimer > AttackSpeed then

        Shoot()
        this:GetAudio():SetPlay()
        AttackTimer = 0
    end
end

function ATTACKinit()
    state = "ATTACK"
    this:GetMeshRenderer():SetMesh("ILY_attack", this) -- Change back to attack animation
    AttackTimer = 0
    Attacked = false
end

function IDLEinit()
    state = "IDLE"
    this:GetMeshRenderer():SetMesh("ILY_Idle", this) -- Change back to idle animation 
end

function Shoot()
    this:GetAudio():SetPlay()
    local pos = Vec3.new()
    pos.x = thisPos.x
    pos.y = thisPos.y
    pos.z = thisPos.z
    pos.y = pos.y + 1
    local bullet = systemManager.ecs:NewEntityFromPrefab("EnemyBullet", pos)
    -- local bulletVec = Helper.Scale(Helper.Normalize(Helper.Vec3Minus(targetPos, thisPos)), ShotSpeed)
    -- phySys:SetVelocity(bullet, bulletVec)
    -- aiSys:SetPredictiveVelocity(bullet, this:GetAISetting():GetTarget(), ShotSpeed)
    aiSys:PredictiveShootPlayer(bullet, ShotSpeed, 30, 50)
    this:GetAnimator():SetFrame(42.06919)
end

-- this function is ran when health just reached 0
function StartDeath()
    -- Start death animation
    -- Start death sound
    state = "DEATH"
    phySys:SetVelocity(this, Vec3.new())
    this:GetMeshRenderer():SetMesh("ILY_Death", this) -- Change to death animation 
    gameStateSys:GetEntity("EnemyDeath"):GetAudio():SetPlay()
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