local this

-- Disable everything (MECHANIC WISE) -> development mode. doing audio
local debug_mode = false
_G.lv3_intro_dialogue_done = false
_G.Boss_Not_Flinchable = false
_G.just_enter_5 = true

local just_enter_3 = true 
local just_enter_2 = true

_G.state_selected = false

local state_internal_chosen = false


-- Phase 1 : Enemyspawn Phase
local enemyType
local enemySpawnDirection
local enemySpawnPosition = Vec3.new()
local currentEnemySpawnResetTimer = 0
local maxEnemySpawnResetTimer = 3
local total_number_of_enemies_to_spawn = 0              -- Total number of enemies to spawn this attack phase
local summon_per_spawn_instance = 0                     -- Per Spawn Instance (2, 3, 2) -> at varying locations
local number_of_enemies_to_summon_per_round = 3          
_G.number_of_spawned_in_level_3 = 0                     -- Keeps track how many enemies have been spawned
_G.number_left_in_level_3 = 0                           -- Keeps track how many enemies is left
_G.Level3_Monsters = true                               -- Special Boolean -> interaction with spawning mechanic
local phase_1_timer = 0 
local phase_1_max_time = 5                             -- Give Players time to fight the minions
local portals = {}                                      -- Table of Portals

-- Phase 1 : Audio Stuff
local roar_audio
local roar_truncated_audio
local initial_roar = false


-- Phase 2 : Groundslam Phase
local groundSlamDirection
local groundSlamPosition = Vec3.new()
local currentGroundSlamResetTimer = 0
local maxGroundSlamResetTimer = 3
local groundSlamCount = 0  -- Changes to other attacking state when it slams a designated amount of time 
local groundSlamMax = 0

local buffer_time = 0.5
local roar_slammed_state = "START"
local roar_anim_timer = 0 -- temp
local slam_audio_timer = 0
local delay_slam_audio_time = 3.5
local s_roared = false
local s_smashed = false

-- Tentative Random Boss State CHanger
local currentBossStateTimer = 0
local maxBossStateTimer = 3.8


-- Phase 3 : Bullethell Phase
local spiralBulletSpawnerObj
local spiralBulletSpawnPosition = Vec3.new()
local bulletSpawnPosition = Vec3.new()
local bulletTranslateRef
local bulletProjectileType = 0                          -- Used to determine which kind of projectile the enemy uses
local fire_rate = 0.5
local fire_timer = 0
local next_fire_time = 1.0

local fire_delay = 2.0
local fire_delay_timer = 0
local phase3_roar_bool = false
local phase3_roar_timer = 0 
local phase3_roar_delay = 0.9

-- Bullet Hell #1 - Circular Pulse
local angles_to_spawn_from = {0, 45 , 90 , 135 , 180}
local no_of_waves_1 = 5                                 -- used to keep track how many waves of bullets to spawn
local random_wave_generator = false                     -- boolean value to keep track if a random value has been generated.
-- keeps track of which bullethell attack has been used (so there won't be back to back usage)

-- [3] Spiral Bullets (Ground)
local bullet_attack_checker = {false, false, false}     -- keep track which attacks have been used so won't activate back to back
_G.attacking = false                                    -- boolean to control when to choose state (used in [BossLaserBeamPhase.lua])
local number_of_fire = 0
local stop_firing_at = 10
local mesh_set_projectile = false

-- [4] Summon Bullet + Homing Bullets (From Boss' Mouth / Front) 
--     -> Players have to shoot them to break them
local spawn_point_ref_obj
local spawn_point_ref_trans
local spawn_point_translate = Vec3.new()                         
local number_of_homing = 0
-- local number_of_homing_spawned = 0
local homing_spawned = false
local homing_spawn_timer = 0                            -- Timer that increament with DT
local homing_spawn_period = 1.0                         -- Time to spawn (in between each sphere)
local homing_spawn_counter = 0                          -- Counter to keep track how many homing bullets have been spawned
local homing_timer = 0 
local homing_state_change = 6.0

local homing_bullet
local next_homing_id = 0                                -- Used in [SpawnHomingSpheres] -> for ID recognition (used for deletion logic)
_G.homing_projectiles = {}                              -- Define a table to store projectile data -> used in [BossBullet.lua] also
local projectile_stay_time = 2                          -- timer for the bullet to stay still before it starts homing into the player
local initial_homing_speed = 8                         -- Starting Homing Speed


-- [5] Lazer Attack 
_G.activateLazerScript = false
local mesh_set_laser = false
local phase5_roar_delay = 0.9
_G.phase5_roar_timer = 0
_G.phase5_roar_bool = false
local play_laser_audio = false
local play_laser_delay = 1.5
_G.play_laser_timer = 0

local lazer_appear_bool = false
_G.lazer_appear_timer = 0
local lazer_appear_delay = 1.5

-- Boss states
local state = 0
-- State Checker List -> checks if this attack has been used (if used, won't repeat again until a certain conditions hits)
-- [1] - Summon Minions -> visual : through a portal  (OK)
-- [2] - Ground Slam (AOE)                            (OK)  -> need to review
-- [3] - Projectile #1 (Pulsing Spheres)              (OK)
-- [4] - Projectile #2 (Homing Eyeballs)              (OK)  -> need to fix homing logic
-- [5] - Lazer Attack (Ground)      

_G.state_checker = {false, false , false , false , false}   -- used in [BossLaserBeamPhase.lua]
local once = false

-- Player Stuff
local player_object 
local player_position = Vec3.new()

-- 1. Summon enemies
-- 2. Ground slam: Boss swings arms and slams the ground, spawning a ground slam area that damages player
-- 3. Shoot projectiles
-- 4. Laser Attack

-- Audio Stuff
local portal_audio 
local roar_slam_audio 
local boss_slam_audio
local sphere_phase_audio

local play_sphere_audio



local homing_audio

-- Health Stuff
local health_bar 
local boss_dead = false



function Alive()
    print("ALIVE")
    this = Helper.GetScriptEntity(script_entity.id)
    gameStateSys = systemManager:mGameStateSystem();
    physicsSys = systemManager:mPhysicsSystem()
    play_sphere_audio = false
    play_laser_audio = false
    -- Testing (Bullet Spawn Position)
    spiralBulletSpawnerObj = gameStateSys:GetEntityByScene("Spiral_Bullet_Spawn", "BossStuff")
    spiralBulletSpawnPosition = spiralBulletSpawnerObj:GetTransform().mTranslate
    spawn_point_ref_obj = gameStateSys:GetEntityByScene("Spawn_Point_Ref" , "BossStuff")
    spawn_point_ref_trans = spawn_point_ref_obj:GetTransform().mTranslate

    -- Player 
    player_object = gameStateSys:GetEntityByScene("Camera" , "test3")
    player_position = player_object:GetTransform().mTranslate
    

    -- For [1] Spiral Bullets (Ground)
    bulletSpawnPosition.x = spiralBulletSpawnPosition.x
    bulletSpawnPosition.y = spiralBulletSpawnPosition.y
    bulletSpawnPosition.z = spiralBulletSpawnPosition.z

    -- For [2]  Summon Bullet + Homing Bullets (From Boss' Mouth / Front) 
    spawn_point_translate.x = spawn_point_ref_trans.x
    spawn_point_translate.y = spawn_point_ref_trans.y
    spawn_point_translate.z = spawn_point_ref_trans.z

    -- print("1")
    -- print("1: " , spawn_point_translate.x)
    -- print("2: " , spawn_point_translate.y)
    -- print("3: " , spawn_point_translate.z)

    -- Audio Stuff
    portal_audio = gameStateSys:GetEntity("SummonPortalAudio")
    roar_audio = gameStateSys:GetEntity("RoarAudio")
    roar_truncated_audio = gameStateSys:GetEntity("RoarTruncatedAudio")
    roar_slam_audio = gameStateSys:GetEntity("RoarSlamAudio")
    boss_slam_audio = gameStateSys:GetEntity("BossSlamAudio")
    sphere_phase_audio = gameStateSys:GetEntity("SpherePhase")
    homing_audio = gameStateSys:GetEntity("HomingEyeballAudio")

    
    -- debug_mode = true

end

function Update()
    health_bar = this:GetHealthbar()

    if health_bar ~= nil then 
        -- print("BOSS HAS HEALTHBAR")
        if health_bar.health <= 0 then 
            -- print("BOSS DED")
            boss_dead = true
        end
    end 

    if state~= 3 then
        play_sphere_audio = false
    end
    if state~= 5 then 
        play_laser_audio = false
        laser_phase = gameStateSys:GetEntity("LaserPhase")
        laserPhaseAudio = laser_phase:GetAudio()
        laserPhaseAudio:SetStop()
    end
    -- Tentative random switcher between boss states, replace with HP after other states implemented. 100% HP Left = Phase 1, 66% HP Left = Phase 2, 33% HP Left = Phase 3
    if _G.attacking == false and _G.FreezePlayerControl  == false and debug_mode == false and boss_dead == false then 

        if _G.state_selected == false then 
            -- [Recycle Attacks]
            -- Check if the current state checker has at least 4 "true" -> reset them 
            local state_true_counter = 0 


            for i = 1, #_G.state_checker do
                if _G.state_checker[i] == true then 
                    state_true_counter = state_true_counter + 1
                 
                end 
            end
            print("STATE TRUE COUNTER: " , state_true_counter)

            if state_true_counter >= 4 then 
                for i = 1, #_G.state_checker do
                    if _G.state_checker[i] == false then 
                        state = i 
                        print("STATE CHOSEN: " , state)
                    end 
                end

                for i = 1, #_G.state_checker do
                    _G.state_checker[i] = false 
                end

                PrintAttackingStates()
                
            else 
                state = math.random(1, 5)
                while _G.state_checker[state] == true do
                    state = math.random(1, 5)
                end
           
                print("STATE CHOSEN: " , state)
            end

            _G.state_selected = true
        end


        -- currentBossStateTimer = 0
    end

    --  Added [3/11] -> to disable when cutscene is on 
    if _G.lv3_intro_dialogue_done == true and _G.level3intro == false and debug_mode == false and boss_dead == false then

        -- print("RUNNING NOT SUPPOSED TO")
         -- Debug States
        -- state = 1 --[OK]
        -- state = 2 -- [OK] -- need to check agn after i check the other mechanics
        -- state = 3 -- [OK]
        -- state = 4 --[OK]
        -- state = 5 -- [OK]

        if state == 1 and _G.state_checker[1] == false then

            -- if just_enter_1 == true then 
            --     _G.Boss_Not_Flinchable = true
            --     just_enter_1 = false
            -- end

            _G.attacking = true -- must include (to stop state choosing)

            -- Decide how many enemies to spawn this phase.
            if total_number_of_enemies_to_spawn == 0 then 
                total_number_of_enemies_to_spawn = math.random(2,4)
                print("NUMBER OF ENEMIES TO SPAWN: " , total_number_of_enemies_to_spawn)
            end

            -- Timer to set intervals between summons
            currentEnemySpawnResetTimer = currentEnemySpawnResetTimer + FPSManager.GetDT()
            -- print("TIMER: " , currentEnemySpawnResetTimer)

            if (currentEnemySpawnResetTimer >= maxEnemySpawnResetTimer) then

                -- Number of enemies to summon per position
                
                if(summon_per_spawn_instance == 0) then 
                    summon_per_spawn_instance = math.random(2,3)
                    -- print("SUMMON PER SPAWN INSTANCE: " , summon_per_spawn_instance)
                end

                -- print("CURRENTLY ENEMIES IN LV3: " , _G.number_of_spawned_in_level_3)
                -- print("TOTAL: " ,_G.number_of_spawned_in_level_3 + summon_per_spawn_instance)

                if(_G.number_of_spawned_in_level_3 +  summon_per_spawn_instance < total_number_of_enemies_to_spawn) then 
                    
                --    print("NORMAL SUMMON")
                -- Animate Roar
                -- if initial_roar == false then
                --     this:GetMeshRenderer():SetMesh("Boss_Roar", this)
                --     roar_audio:GetAudio():SetPlay(1.0)
                --     initial_roar = true
                -- end

                SummonMinions(summon_per_spawn_instance)
                currentEnemySpawnResetTimer = 0 -- Reset spawn time

                else -- if exceed the total amount. 
                            
                    -- print("SPECIAL SUMMON")
                    -- Animate Roar
                    -- if initial_roar == false then
                    --     this:GetMeshRenderer():SetMesh("Boss_Roar", this)
                    --     roar_audio:GetAudio():SetPlay(1.0)
                    --     initial_roar = true
                    -- end
                    if (total_number_of_enemies_to_spawn - _G.number_of_spawned_in_level_3 > 0 ) then 
                        SummonMinions(total_number_of_enemies_to_spawn - _G.number_of_spawned_in_level_3)
                        currentEnemySpawnResetTimer = 0 -- Reset spawn time
                    end
                end

                summon_per_spawn_instance   = 0 -- Reset number of enemies per spawn instance (for a new RNG) -> put here coz might exceed total number    
            end

            if(_G.number_of_spawned_in_level_3 >= total_number_of_enemies_to_spawn) then  -- Exit State (Condition)
                phase_1_timer = phase_1_timer + FPSManager.GetDT()
                -- print("PHASE TIMER: " , phase_1_timer)
            end

            if(phase_1_timer >= phase_1_max_time) then 
                -- just_enter_1 = true
                -- print("SWITCHING OUT PHASE 1")
                _G.state_checker[1] = true 
                _G.attacking = false           -- attack done (exit state)
                PrintAttackingStates()
                phase_1_timer = 0              -- reset toimer
                initial_roar = false
                _G.state_selected = false
                currentEnemySpawnResetTimer = 0
                _G.number_of_spawned_in_level_3 = 0
            end

        end

        if state == 2 and _G.state_checker[2] == false then 

            if just_enter_2 == true then 
                _G.Boss_Not_Flinchable = true
                just_enter_2 = false
            end

            if roar_slammed_state == "RS_SLAM" then 
                this:GetMeshRenderer():SetMesh("Boss_Slam" , this)
                roar_slammed_state = "SLAM_ANIM"
            end

            if roar_slammed_state == "SLAM_ANIM" then 
                slam_audio_timer = slam_audio_timer + FPSManager.GetDT()

                if slam_audio_timer >= delay_slam_audio_time then
                    roar_slammed_state = "PLAY_SLAM_AUDIO"
                end
            end

            if roar_slammed_state == "PLAY_SLAM_AUDIO" then -- delayed audio slam
                 boss_slam_audio:GetAudio():SetPlay(1.0)
                 roar_slammed_state = "SLAMMING"
            end

            if roar_slammed_state == "SLAMMING" then 
                -- if currentGroundSlamResetTimer >= maxGroundSlamResetTimer then

                    -- Pick which direction to ground slam in 
                    groundSlamDirection = math.random(1, 8)
                    -- groundSlamDirection  = 2

                    -- print("GROUND SLAM: " , groundSlamDirection)

                    -- Ground slam (9 o'clock)
                    if groundSlamDirection == 1 then
                        groundSlamPosition.x = 0
                        groundSlamPosition.y = 0
                        groundSlamPosition.z = 40
                    end

                    -- Ground slam (7 o'clock)
                    if groundSlamDirection == 2 then
                        groundSlamPosition.x = 37   
                        groundSlamPosition.y = 0
                        groundSlamPosition.z = 37
                    end

                    -- Ground slam (6 o'clock)
                    if groundSlamDirection == 3 then
                        groundSlamPosition.x = 50
                        groundSlamPosition.y = 0
                        groundSlamPosition.z = 8
                    end

                    -- Ground slam (5 o'clock)
                    if groundSlamDirection == 4 then
                        groundSlamPosition.x = 33
                        groundSlamPosition.y = 0
                        groundSlamPosition.z = -32
                    end

                    -- Ground slam (3 o'clock)
                    if groundSlamDirection == 5 then
                        groundSlamPosition.x =  3
                        groundSlamPosition.y = 0
                        groundSlamPosition.z = -42
                    end

                    -- Ground slam (1 o'clock)
                    if groundSlamDirection == 6 then
                        groundSlamPosition.x = -43
                        groundSlamPosition.y = 0
                        groundSlamPosition.z = -29
                    end

                    -- Ground slam (11o'clock)
                    if groundSlamDirection == 7 then
                        groundSlamPosition.x = -43
                        groundSlamPosition.y = 0
                        groundSlamPosition.z = 39
                    end

                    -- Ground slam (12 o'clock)
                    if groundSlamDirection == 8 then
                        groundSlamPosition.x = -60
                        groundSlamPosition.y = 0
                        groundSlamPosition.z = 2.3
                    end

                    -- TODO: Play arm swinging animation before spawning ground slam object
                    roundSlam = systemManager.ecs:NewEntityFromPrefab("GroundSlamObject", groundSlamPosition)
                    groundSlamCount = groundSlamCount + 1
                    -- print("GROUND SLAM COUNT: ", groundSlamCount)
                    -- currentGroundSlamResetTimer = 0 -- Reset ground slam timer

                    roar_slammed_state = "SPAWN_SHOCKWAVE"
            end
          

            _G.attacking = true -- must include (to stop state choosing)

            if groundSlamMax == 0 then 
            
                groundSlamMax = math.random(1,1)
                -- print("GROUND SLAM MAX: " , groundSlamMax)
            end
            -- Timer to set intervals between ground slams
            -- print("Current Ground Slam Reset Timer: " , currentGroundSlamResetTimer)

            if groundSlamCount < groundSlamMax then 
                -- print("HI INSIDE HERE")
                currentGroundSlamResetTimer = currentGroundSlamResetTimer + FPSManager.GetDT()
                -- print("Current Ground Slam Reset Timer: " , currentGroundSlamResetTimer)
                if currentGroundSlamResetTimer + buffer_time >= maxGroundSlamResetTimer then 
                    -- roar_slammed_state = "START"

                    if roar_slammed_state == "START" then 
                        roar_audio:GetAudio():SetPlay(1.0)
                        this:GetMeshRenderer():SetMesh("Boss_Roar", this)
                        roar_slammed_state = "RS_SLAM"
                    end
                end

            else 
                _G.Boss_Not_Flinchable = false
                just_enter_2 = true
                print("DONE SLAM")
                PrintAttackingStates()
                _G.state_checker[2] = true
                _G.attacking = false
                groundSlamMax = 0
                smashed = false 
                _G.state_selected = false
                slam_audio_timer = 0
                currentGroundSlamResetTimer = 0
            end

        end

        -- Bullet Types (Phases)
        -- 1. Bullet Hell (no homing) -> maybe add 2 variations to start
        -- 2. Homing Bullets (but dodgeable)

        
        -- [3] Bullet Hell #1 : Pulsing Spheres
        -- (a) State 1 : Normal Circles but pulsing in different directions & angles. 
        --     - Make it easier at the start 
        if state == 3 and _G.state_checker[3] == false then

            if just_enter_3 == true then 
                _G.Boss_Not_Flinchable = true
                just_enter_3 = false
            end

            if play_sphere_audio == false then
                play_sphere_audio = true
                -- sphere_phase_audio:GetAudio():SetPlay(0.5) 
            end
            _G.attacking = true -- must include (to stop state choosing)

            fire_timer = fire_timer +  FPSManager.GetDT()

            fire_delay = fire_delay + FPSManager.GetDT()

            if phase3_roar_bool == false then 
                phase3_roar_timer = phase3_roar_timer + FPSManager.GetDT()
            end

            if(random_wave_generator == false) then 
                random_wave_generator = true 
                no_of_waves_1 = math.random(5, 8) -- can generate between 5 to 10 waves of bullets
            end

            -- Sync Audio to [Projectile Animation]
            if  phase3_roar_timer >= phase3_roar_delay then 
                roar_truncated_audio:GetAudio():SetPlay(1.0)
                phase3_roar_timer = 0
                phase3_roar_bool = true
            end

            -- Happens once only
            if mesh_set_projectile == false then 
                this:GetMeshRenderer():SetMesh("Boss_Projectile", this)
                mesh_set_projectile = true
            end 

            if this:GetAnimator():GetFrame() >= 85 then 
                this:GetMeshRenderer():SetMesh("Boss_Projectile", this)
                this:GetAnimator():SetFrame(36)
            end

            if fire_delay > fire_delay_timer then  -- to account syncing of animation and projectiles coming out
                if fire_timer > next_fire_time then 
                    if number_of_fire < stop_firing_at then 
                        fire_timer = 0 -- reset timer

                        SpawnBulletsPattern1(no_of_waves_1)
    
                        number_of_fire = number_of_fire + 1
                    else -- Exit State (Condition)
                        just_enter_3 = true
                        phase3_roar_bool = false
                        _G.state_checker[3] = true
                        _G.attacking = false
                        mesh_set_projectile = false
                        PrintAttackingStates()
                        _G.state_selected = false
                        fire_timer = 0
                        fire_delay = 0
                        number_of_fire = 0
                    end
                end
            end
           

        end

        -- [4] Homing Eyeballs (From Boss' Mouth / Face)
        if state == 4 and _G.state_checker[4] == false then 
            -- print("INSIDE STATE 4")
            -- homing_spawn_counter = 0 
            _G.attacking = true -- must include (to stop state choosing)

            -- Initially -> decides the number of homing to spawn
            if number_of_homing == 0 then 
                number_of_homing = math.random(5,8)
                -- print("NUMBER OF HOMING: " , number_of_homing)
            end

            -- Timer to spawn 1 by 1
            if homing_spawn_counter ~= number_of_homing then 
                -- print("SPAWNED ENOUGH")
                homing_spawn_timer = homing_spawn_timer + FPSManager.GetDT()
            else 
                -- print("SPAWNED ENOUGH")
            end

            -- print("HOMING SPAWN TIMER: " , homing_spawn_timer)
            
            if(homing_spawn_counter < number_of_homing) then 
                if(homing_spawn_timer > homing_spawn_period) then -- It's time to spawn another homing bullet
                    SpawnHomingSpheres()
                    homing_spawn_counter = homing_spawn_counter + 1 -- Increase the counter
                    homing_spawn_timer = 0   -- Reset the counter
                end
            else 
                homing_timer = homing_timer + FPSManager.GetDT()
            end

            if homing_timer >= homing_state_change then 
                -- print("CHANGING OUT OF STATE 4 : Homing")
                _G.state_checker[4] = true
                homing_spawn_timer = 0    -- need to reset 
                homing_spawn_counter = 0  -- Reset number of homing spawned
                number_of_homing = 0      -- Reset number of homing required
                homing_timer = 0 
                _G.attacking = false           -- important to toggle state choose again
                PrintAttackingStates()
                _G.state_selected = false
            end
        end


        if state == 5 and _G.state_checker[5] == false then 
            if _G.just_enter_5 == true then 
                _G.Boss_Not_Flinchable = true
                _G.just_enter_5 = false
            end

            if _G.phase5_roar_bool == false then 
                _G.phase5_roar_timer = _G.phase5_roar_timer + FPSManager.GetDT()
            end

            if play_laser_audio == false then 
                _G.play_laser_timer = _G.play_laser_timer + FPSManager.GetDT()
            end

            if lazer_appear_bool == false then 
                _G.lazer_appear_timer = _G.lazer_appear_timer + FPSManager.GetDT()
            end

            -- Sync Audio to [Projectile Animation]
            if  _G.phase5_roar_timer >= phase5_roar_delay then 
                roar_truncated_audio:GetAudio():SetPlay(1.0)
                _G.phase5_roar_timer = 0
                _G.phase5_roar_bool = true
            end

            if mesh_set_laser == false then 
                this:GetMeshRenderer():SetMesh("Boss_Laser", this)
                mesh_set_laser = true
            end

            if play_laser_timer >= play_laser_delay then
                laser_phase:GetAudio():SetPlay(0.5)
                play_laser_audio = true
                play_laser_timer = 0
            end
            
            if _G.lazer_appear_timer >= lazer_appear_delay then 
                -- print("LAZER UP PLEASE")
                _G.attacking = true -- must include (to stop state choosing)
                _G.activateLazerScript = true
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

function randomnizeState() 
   local random_state = math.random(1, 3)


end 

-- Function to rotate a vector around the Y-axis
-- angle is in degrees
function RotateVectorAroundYAxis(vector, angle)
    local angleInRadians = math.rad(angle)
    local rotatedX = vector.x * math.cos(angleInRadians) - vector.z * math.sin(angleInRadians)
    local rotatedZ = vector.x * math.sin(angleInRadians) + vector.z * math.cos(angleInRadians)
    -- return { x = rotatedX, y = vector.y, z = rotatedZ }

    vector.x = rotatedX 
    vector.z = rotatedZ
    -- print("ROTATED X: " , vector.x)
    -- print("ROTATED Z: " , vector.z)
end

-- local Vec3 = {}

-- Define Subtract method within Vec3 module
function Subtract(vector1, vector2)
    local result = Vec3.new()

    result.x = vector1.x - vector2.x
    result.y = vector1.y - vector2.y
    result.z = vector1.z - vector2.z

    return result
end

-- Bullet Attack 1 - Spiraling Spheres (Ground)
function SpawnBulletsPattern1(number_of_bullets)

    local angleStep = 360 / number_of_bullets
    -- Add some randomnization
    local random_index = math.random(1,5)
    local angle = angles_to_spawn_from[random_index]
    -- print("ANGLE: " , angle)

    -- local angle = 0

    local forward = Vec3.new(0,0,1)

    -- print("NUMBER OF BULLETS TO SPAWN")

    for i = 0 , number_of_bullets do 

        bulletDirection = Vec3.new(1,0,0)
        bulletSpeed = 12
        -- Caluclate direction vector in 3D space
        RotateVectorAroundYAxis(bulletDirection, angle) -- Testing
        -- print("X: " , bulletDirection.x , " Y: " , bulletDirection.y , " Z: ", bulletDirection.z)
        sphere_bullet = systemManager.ecs:NewEntityFromPrefab("Boss_Bullet", bulletSpawnPosition)
        -- print("Spawning at: " , bulletSpawnPosition.x , ", " , bulletSpawnPosition.y , ", " , bulletSpawnPosition.z)

        -- Applying speed to vector
        bulletDirection.x = bulletDirection.x * bulletSpeed 
        bulletDirection.y = bulletDirection.y * bulletSpeed 
        bulletDirection.z = bulletDirection.z * bulletSpeed 

        physicsSys:SetVelocity(sphere_bullet, bulletDirection)
        
        angle = angle + angleStep
        
    end

    sphere_phase_audio:GetAudio():SetPlay(0.3) 

end

-- Bullet Attack 2 - Homing Spheres (From eyes to player)
function SpawnHomingSpheres()
    -- Let's start by randomnizing the starting spawn point of the projectile
    homing_audio:GetAudio():SetPlay(0.5)
    -- Spawning Logic (Start)
    -- for i = 0 , number_of_bullets do 
    local attack_offset_range = 5  -- Defines a range to spread out from the central position to summon the bullets in different positions
                
    local random_offset_x = math.random(-attack_offset_range , attack_offset_range)
    local random_offset_y = math.random(-attack_offset_range , attack_offset_range)
    local random_offset_z = math.random(-attack_offset_range , attack_offset_range)
    
    local bullet_spawn_position = Vec3.new(spawn_point_translate.x + random_offset_x , 
                                        spawn_point_translate.y + random_offset_y , 
                                        spawn_point_translate.z + random_offset_z)

    -- homing_bullet = systemManager.ecs:NewEntityFromPrefab("Boss_Bullet_Homing",bullet_spawn_position)
    entity_ref = systemManager.ecs:NewEntityFromPrefab("Boss_Bullet_Homing" , bullet_spawn_position)

    -- homing_spawned = true -- to trigger "Update" Loop
end

-- [M6 - 4/1] - Might Not Need anymore
function UpdateHomingProjectiles()
    -- print("UPDATE HOMING")
end 

function SummonMinions(summon_per_spawn_instance) 
    -- print("Number of Enemies to Summon: " , summon_per_spawn_instance)

    portal_audio:GetAudio():SetPlay(0.7)

   

    -- Pick which direction to ground slam in 
    enemySpawnDirection = math.random(1, 4)
    enemySpawnDirection = 1
    -- print("ENEMY SPAWN DIRECTION: " , enemySpawnDirection)

    -- Summon Area (In Front of Boss)
    if enemySpawnDirection == 1 then
        groundSlamPosition.x = 43.2
        groundSlamPosition.y = 0.697
        groundSlamPosition.z = 7
    end

    -- Summon Area (Left of Boss)
    if enemySpawnDirection == 2 then
        groundSlamPosition.x = 0
        groundSlamPosition.y = 0
        groundSlamPosition.z = 40

    end

    -- Summon Area (Right of Boss)
    if enemySpawnDirection == 3 then
        groundSlamPosition.x = 5.6  
        groundSlamPosition.y = 0
        groundSlamPosition.z = -35
    end

    -- Summon Area (Behind Boss)
    if enemySpawnDirection == 4 then
        groundSlamPosition.x = -33.7
        groundSlamPosition.y = 0
        groundSlamPosition.z = -0.09
    end

    -- Summon Portal (Visual)
    portal = systemManager.ecs:NewEntityFromPrefab("Portal", groundSlamPosition)
    
    

    -- [3/6] - Set to summon multiple enemies in 1 area 
    for i = 1 , summon_per_spawn_instance  do 
        enemyType = math.random(1, 4)
        -- print("ENEMY TYPE: " , enemyType)
        -- print("SUMMON # " , _G.number_of_spawned_in_level_3)
        _G.number_of_spawned_in_level_3 = _G.number_of_spawned_in_level_3 + 1
        _G.number_left_in_level_3 = _G.number_left_in_level_3 + 1

        if enemyType == 1 then
            enemySpawn = systemManager.ecs:NewEntityFromPrefab("Melissa", groundSlamPosition)
        end

        if enemyType == 2 then
            enemySpawn = systemManager.ecs:NewEntityFromPrefab("ILOVEYOU", groundSlamPosition)
        end

        if enemyType == 3 then
            enemySpawn = systemManager.ecs:NewEntityFromPrefab("ZipBomb", groundSlamPosition)
        end

        if enemyType == 4 then
            enemySpawn = systemManager.ecs:NewEntityFromPrefab("TrojanHorse", groundSlamPosition)
        end

    end
end


function PrintAttackingStates()
    for i = 1, #_G.state_checker do
        print("Phase " .. i .. ": " .. tostring(_G.state_checker[i]))
    end

    print("ATTACKING: " , _G.attacking)
end


function PortalAnimation()

    -- 0.13 , -0.06, 7.53
    -- 0.13 , 12.1 , 7.53
    portal_trans = portal:GetTransform()

    portal_open_timer = 0
    portal_max_timer = 3

end

-- function UpdateHomingProjectiles()
--     for i , projectiles in

-- end

function crossProduct(v1, v2)
    local x = v1.y * v2.z - v1.z * v2.y
    local y = v1.z * v2.x - v1.x * v2.z
    local z = v1.x * v2.y - v1.y * v2.x
    
    return {x = x, y = y, z = z}
end

function dotProduct(v1, v2)
    local result = v1.x * v2.x + v1.y * v2.y + v1.z * v2.z
    return result
end

-- M6 : Calculate Homing Bullet's Orientation (angle to rotate) -> bullet to player
function CalculateAngle(bullet_pos, player_position, forward_vector)

    -- Calculate the direction vector from bullet to player
    local direction = Vec3.new()

    direction.x = player_position.x - bullet_pos.x 
    direction.y = player_position.y - bullet_pos.y 
    direction.z = player_position.z - bullet_pos.z

    -- Normalize the vector/direction
    local directionNorm = Helper.Normalize(direction)

    -- Calculates the dot product between [bullet's forward direction] & [direction from bullet to player - normalized]
    local dot_product_forward = dotProduct(forward_vector, directionNorm) 

 
    -- Calculate the angle between the bullet's forward direction & the direction vector.
    -- [forward direction] -> represents the object's orientation or where it is facing. 
    local angle = math.acos(dot_product_forward)

    -- Use <Cross Product> to determine if angle is positive / negative
    local cross = crossProduct(forward_vector, directionNorm)
    -- print("ANGLE: " , angle * (180 / math.pi))

    local l = dotProduct(cross, cross)
    
    --  if l < 0.01 then 
    --     cross = Vec3.new(0,1,0)
    --  end

    -- print("DOT PRODUCT FORWARD: " , dot_product_forward)

    -- if 
    -- print("L: " , l)
    --  if dot_product_forward < 0 then  
    --     angle = angle - (math.pi / 2)
    --  end

--    if cross.y < 0 then 
--        angle = -angle
--    end
    -- Temp Fix
    -- if dot_product_forward

    return angle
end