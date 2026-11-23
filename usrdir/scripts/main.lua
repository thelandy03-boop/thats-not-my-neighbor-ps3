-- That's Not My Neighbor - PS3 Demake
-- main.lua: Core Game Loop, State Controller, and Virtual Cursor

local neighbors = require("neighbors")

-- Game States
local STATES = {
    WAIT_NEIGHBOR   = 1, -- Shutter closed, waiting for next tenant
    EXAMINE         = 2, -- Shutter open, neighbor outside, papers on desk
    INSPECT_DOC     = 3, -- Examining ID Card or Entry Request up close
    INSPECT_LIST    = 4, -- Inspecting Today's List
    PHONE_CALL      = 5, -- Rotary phone interface to dial DDD (3312)
    EMERGENCY_ALARM = 6, -- Alarm siren, DDD cleaning protocol active
    DOPPEL_ATTACK   = 7, -- Player made a fatal mistake!
    SUMMARY_VICTORY = 8  -- All visitors cleared successfully
}

-- Global Game Context
local game = {
    running = true,
    state = STATES.WAIT_NEIGHBOR,

    -- Virtual Cursor
    cursor = {
        x = 640.0,
        y = 360.0,
        speed = 12.0,
        clicking = false
    },
    pad_prev = {},

    -- Security Booth Elements
    shutter_open = false,
    alarm_cover_open = false,
    alarm_active = false,
    alarm_timer = 0.0,

    -- Active Tenant / Visitor
    current_visitor = nil,
    visitor_queue = 5,
    visitors_cleared = 0,
    doppels_eliminated = 0,
    innocents_killed = 0,

    -- Today's Approved Entry List
    todays_list = {},

    -- Phone Context
    phone = {
        dialed = "",
        calling = false,
        response_text = ""
    },

    -- UI Hitboxes (X, Y, Width, Height)
    hitboxes = {
        shutter_toggle = { x = 60,  y = 480, w = 110, h = 90  },
        door_unlock    = { x = 60,  y = 590, w = 110, h = 50  },
        door_lock      = { x = 60,  y = 650, w = 110, h = 50  },
        phone_desk     = { x = 200, y = 500, w = 140, h = 180 },
        alarm_cover    = { x = 370, y = 520, w = 70,  h = 50  },
        alarm_button   = { x = 370, y = 580, w = 70,  h = 70  },
        desk_id        = { x = 500, y = 550, w = 130, h = 90  },
        desk_request   = { x = 650, y = 540, w = 140, h = 110 },
        desk_list      = { x = 820, y = 530, w = 150, h = 130 }
    },

    -- Phone Rotary Dial Buttons (0-9, Hangup)
    phone_dial_buttons = {
        [1] = { x = 500, y = 260, w = 60, h = 60, digit = "1" },
        [2] = { x = 610, y = 260, w = 60, h = 60, digit = "2" },
        [3] = { x = 720, y = 260, w = 60, h = 60, digit = "3" },
        [4] = { x = 500, y = 340, w = 60, h = 60, digit = "4" },
        [5] = { x = 610, y = 340, w = 60, h = 60, digit = "5" },
        [6] = { x = 720, y = 340, w = 60, h = 60, digit = "6" },
        [7] = { x = 500, y = 420, w = 60, h = 60, digit = "7" },
        [8] = { x = 610, y = 420, w = 60, h = 60, digit = "8" },
        [9] = { x = 720, y = 420, w = 60, h = 60, digit = "9" },
        [0] = { x = 610, y = 500, w = 60, h = 60, digit = "0" },
        hangup = { x = 500, y = 500, w = 80, h = 50, label = "Colgar" }
    },

    message_banner = "TURNO INICIADO: REVISE DOCUMENTOS Y CONTACTE AL D.D.D. ANTE CUALQUIER ANOMALIA",
    banner_timer = 300
}

-- Point-in-Rectangle Collision Helper
local function point_in_rect(px, py, rect)
    return px >= rect.x and px <= (rect.x + rect.w) and
           py >= rect.y and py <= (rect.y + rect.h)
end

-- Initialize Game
function game_init()
    log_info("Starting That's Not My Neighbor PS3 Demake...")
    game.todays_list = neighbors.generate_todays_list()
    play_music("Music_Dark.ogg", true)

    -- Spawn first visitor (first visitor is always legitimate Roman or Francis)
    game.current_visitor = neighbors.spawn_visitor(false)
end

function game_is_running()
    return game.running
end

-- Spawn Next Tenant in Queue
local function spawn_next()
    if game.visitor_queue <= 0 then
        game.state = STATES.SUMMARY_VICTORY
        game.message_banner = "TURNO COMPLETADO: TODOS LOS RESIDENTES HAN SIDO EVALUADOS."
        return
    end

    game.visitor_queue = game.visitor_queue - 1
    -- 50% chance of being a doppelgänger
    local is_doppel = (math.random() > 0.5)
    game.current_visitor = neighbors.spawn_visitor(is_doppel)
    game.shutter_open = false
    game.state = STATES.WAIT_NEIGHBOR
    game.message_banner = "NUEVO INDIVIDUO EN LA PUERTA. ABRA LA VENTANILLA PARA EVALUAR."
    game.banner_timer = 240
    play_sfx("SFX_Window.wav")
end

-- Trigger DDD Purge Alarm
local function trigger_alarm()
    if not game.current_visitor then return end

    play_sfx("SFX_Siren.ogg")
    game.alarm_active = true
    game.alarm_timer = 180 -- ~3 seconds
    game.state = STATES.EMERGENCY_ALARM

    local is_fake = game.current_visitor.is_doppel
    if is_fake then
        game.doppels_eliminated = game.doppels_eliminated + 1
        game.message_banner = "D.D.D. HA EJECUTADO AL DOPPELGANGER EXITOSAMENTE."
    else
        game.innocents_killed = game.innocents_killed + 1
        game.message_banner = "ALERTA: HAS ELIMINADO A UN INQUILINO INOCENTE."
    end
end

-- Allow Entry
local function allow_entry()
    if not game.current_visitor then return end

    if game.current_visitor.is_doppel then
        -- You let a monster in! Game over attack sequence
        play_sfx("SFX_Window_Break.wav")
        play_sfx("SFX_Bite.wav")
        game.state = STATES.DOPPEL_ATTACK
        game.message_banner = "¡HAS DEJADO ENTRAR A UN DOPPELGANGER! LA MUERTE ES INMINENTE."
    else
        play_sfx("SFX_Accept.wav")
        play_sfx("SFX_Open_Door.wav")
        game.visitors_cleared = game.visitors_cleared + 1
        game.message_banner = "ACCESO PERMITIDO. DOCUMENTOS VALIDADOS."
        spawn_next()
    end
end

-- Main Update Loop (60 FPS)
function game_update()
    local pad = get_pad_input()

    -- 1. Virtual Cursor Movement via Left Analog Stick
    if math.abs(pad.analog_lx) > 0.05 then
        game.cursor.x = game.cursor.x + (pad.analog_lx * game.cursor.speed)
    end
    if math.abs(pad.analog_ly) > 0.05 then
        game.cursor.y = game.cursor.y + (pad.analog_ly * game.cursor.speed)
    end

    -- Clamp cursor to 720p screen bounds
    if game.cursor.x < 10.0  then game.cursor.x = 10.0 end
    if game.cursor.x > 1270.0 then game.cursor.x = 1270.0 end
    if game.cursor.y < 10.0  then game.cursor.y = 10.0 end
    if game.cursor.y > 710.0 then game.cursor.y = 710.0 end

    -- Detect Single Click Edge (Cross button)
    local clicked = pad.cross and not game.pad_prev.cross
    local cancelled = pad.circle and not game.pad_prev.circle

    if game.banner_timer > 0 then
        game.banner_timer = game.banner_timer - 1
    end

    -- 2. State-Specific Updates
    if game.state == STATES.EMERGENCY_ALARM then
        game.alarm_timer = game.alarm_timer - 1
        if game.alarm_timer <= 0 then
            game.alarm_active = false
            spawn_next()
        end

    elseif game.state == STATES.DOPPEL_ATTACK then
        if clicked or cancelled then
            -- Restart on button press
            game.visitor_queue = 5
            game.visitors_cleared = 0
            game.doppels_eliminated = 0
            game.innocents_killed = 0
            spawn_next()
        end

    elseif game.state == STATES.SUMMARY_VICTORY then
        if clicked then
            game.visitor_queue = 5
            spawn_next()
        end

    elseif game.state == STATES.INSPECT_DOC or game.state == STATES.INSPECT_LIST then
        -- Circle button closes the document inspection
        if cancelled or clicked then
            play_sfx("SFX_Pick_Up_Folder.wav")
            game.state = STATES.EXAMINE
        end

    elseif game.state == STATES.PHONE_CALL then
        -- Circle button hangs up
        if cancelled then
            play_sfx("SFX_Hang_Down_Phone.wav")
            game.state = STATES.EXAMINE
        end

        if clicked then
            for _, btn in pairs(game.phone_dial_buttons) do
                if point_in_rect(game.cursor.x, game.cursor.y, btn) then
                    if btn.digit then
                        if #game.phone.dialed < 4 then
                            game.phone.dialed = game.phone.dialed .. btn.digit
                            play_sfx("SFX_Dial_1.wav")

                            if game.phone.dialed == "3312" then
                                -- D.D.D. Hotline Reached!
                                play_sfx("SFX_Phone_Ring.ogg")
                                game.phone.response_text = "D.D.D.: PROTOCOLO 3312 CONFIRMADO. SI SOSPECHA, ACTIVE ALARMA."
                            end
                        end
                    elseif btn.label == "Colgar" then
                        play_sfx("SFX_Hang_Down_Phone.wav")
                        game.phone.dialed = ""
                        game.phone.response_text = ""
                        game.state = STATES.EXAMINE
                    end
                end
            end
        end

    else
        -- Active security desk interaction
        if clicked then
            -- Toggle Shutter
            if point_in_rect(game.cursor.x, game.cursor.y, game.hitboxes.shutter_toggle) then
                game.shutter_open = not game.shutter_open
                play_sfx("SFX_Window.wav")
                if game.shutter_open then
                    game.state = STATES.EXAMINE
                    game.message_banner = "VENTANILLA ABIERTA. OBSERVE ATENTAMENTE EL ROSTRO Y DOCUMENTOS."
                else
                    game.state = STATES.WAIT_NEIGHBOR
                    game.message_banner = "VENTANILLA CERRADA."
                end
            end

            -- Door Unlock (Allow entry)
            if point_in_rect(game.cursor.x, game.cursor.y, game.hitboxes.door_unlock) then
                allow_entry()
            end

            -- Door Lock (Deny entry / send away)
            if point_in_rect(game.cursor.x, game.cursor.y, game.hitboxes.door_lock) then
                play_sfx("SFX_Select.wav")
                game.message_banner = "ACCESO DENEGADO. RESIDENTE DESPACHADO."
                spawn_next()
            end

            -- Inspect ID Card
            if point_in_rect(game.cursor.x, game.cursor.y, game.hitboxes.desk_id) then
                play_sfx("SFX_Pick_Up_Folder.wav")
                game.state = STATES.INSPECT_DOC
            end

            -- Inspect Entry Request
            if point_in_rect(game.cursor.x, game.cursor.y, game.hitboxes.desk_request) then
                play_sfx("SFX_Pick_Up_Folder.wav")
                game.state = STATES.INSPECT_DOC
            end

            -- Inspect Today's List
            if point_in_rect(game.cursor.x, game.cursor.y, game.hitboxes.desk_list) then
                play_sfx("SFX_Pick_Up_Folder.wav")
                game.state = STATES.INSPECT_LIST
            end

            -- Open Rotary Phone
            if point_in_rect(game.cursor.x, game.cursor.y, game.hitboxes.phone_desk) then
                play_sfx("SFX_Pick_Up_Phone.wav")
                play_sfx("SFX_Dial_Tone.ogg")
                game.phone.dialed = ""
                game.phone.response_text = "MARQUE 3312 PARA COMUNICARSE CON EL D.D.D."
                game.state = STATES.PHONE_CALL
            end

            -- Alarm Cover Toggle
            if point_in_rect(game.cursor.x, game.cursor.y, game.hitboxes.alarm_cover) then
                game.alarm_cover_open = not game.alarm_cover_open
                play_sfx("SFX_Select.wav")
            end

            -- Alarm Red Button
            if game.alarm_cover_open and point_in_rect(game.cursor.x, game.cursor.y, game.hitboxes.alarm_button) then
                trigger_alarm()
            end
        end
    end

    -- Save previous pad state for single-click detection
    game.pad_prev = pad
end

-- Render Security Booth Background & Elements
local function render_booth()
    -- Base background room
    draw_sprite("Background.png", 0, 0, 1280, 720)

    -- If shutter is open, draw the visitor outside the window
    if game.shutter_open and game.current_visitor then
        local vis = game.current_visitor
        local tx = 520
        local ty = 140

        -- Render Character Components
        if vis.template.body_asset then
            if vis.physical_blood then
                draw_sprite("Char_Francis_Body_Blood.png", tx, ty + 120, 240, 260)
            else
                draw_sprite(vis.template.body_asset, tx, ty + 120, 240, 260)
            end
        end
        if vis.template.head_asset then
            draw_sprite(vis.template.head_asset, tx + 20, ty, 200, 190)
        end
        if vis.template.eyes_asset then
            draw_sprite(vis.template.eyes_asset, tx + 40, ty + 50, 160, 40)
        end
        if vis.template.nose_asset then
            draw_sprite(vis.template.nose_asset, tx + 70, ty + 70, 60, 60)
        end
        if vis.template.mouth_asset then
            draw_sprite(vis.template.mouth_asset, tx + 60, ty + 130, 80, 40)
        end
        if vis.template.hat_asset then
            draw_sprite(vis.template.hat_asset, tx + 10, ty - 50, 220, 110)
        end

        -- Anomalous physical features
        if vis.physical_extra_eye then
            draw_sprite("Char_Roman_Doppel_3rd_eye.png", tx + 75, ty + 20, 50, 40)
        end
        if vis.physical_uzumaki then
            draw_sprite("Char_Roman_Doppel_Uzumaki_Face.png", tx + 20, ty, 200, 190)
        end
    end

    -- Booth Wall, Hatch frame, and Window
    draw_sprite("Wall.png", 0, 0, 1280, 720)
    if not game.shutter_open then
        draw_sprite("Panic_window.png", 420, 90, 440, 390)
    end

    -- Door Control Panel on Desk
    draw_sprite("Door_control.png", 40, 460, 150, 250)
    draw_sprite("Door_button_unlock.png", 60, 590, 110, 50)
    draw_sprite("Door_button_lock.png", 60, 650, 110, 50)

    -- Rotary Phone on Desk
    draw_sprite("Phone_base.png", 200, 500, 140, 180)

    -- Emergency Alarm Box
    draw_rect(365, 510, 80, 150, 80, 20, 20, 255)
    draw_text(370, 515, "DDD ALARM", 255, 255, 255, 255)
    if game.alarm_cover_open then
        draw_rect(370, 580, 70, 70, 220, 30, 30, 255)
        draw_text(385, 605, "PULSAR", 255, 255, 255, 255)
    else
        draw_rect(370, 520, 70, 130, 180, 180, 50, 200)
        draw_text(375, 570, "TAPA SEG", 0, 0, 0, 255)
    end

    -- Documents on Desk
    if game.current_visitor then
        draw_sprite("ID.png", 500, 550, 130, 90)
        draw_sprite("Paper.png", 650, 540, 140, 110)
    end
    draw_sprite("Todays_list.png", 820, 530, 150, 130)

    -- DDD Logo Watermark
    draw_sprite("DDD_logo.png", 1120, 20, 130, 130)

    -- Shutter Lever Status
    if game.shutter_open then
        draw_rect(65, 490, 100, 30, 50, 200, 50, 255)
        draw_text(75, 498, "VENTANILLA ABIERTA", 0, 0, 0, 255)
    else
        draw_rect(65, 490, 100, 30, 200, 50, 50, 255)
        draw_text(75, 498, "VENTANILLA CERRADA", 255, 255, 255, 255)
    end
end

-- Render Document Inspection Overlay
local function render_doc_inspection()
    draw_rect(100, 50, 1080, 620, 10, 10, 15, 240)
    draw_rect(120, 70, 1040, 580, 240, 230, 200, 255)

    local vis = game.current_visitor
    if not vis then return end

    -- Draw ID Card Inspection
    draw_sprite("ID.png", 160, 120, 460, 320)
    if vis.template.photo_asset then
        draw_sprite(vis.template.photo_asset, 190, 170, 130, 160)
    end

    draw_text(340, 180, "NOMBRE: " .. vis.name, 20, 20, 20, 255)
    draw_text(340, 215, "APELLIDO: " .. vis.last_name, 20, 20, 20, 255)
    draw_text(340, 250, "ID: " .. vis.displayed_id, 20, 20, 20, 255)
    draw_text(340, 285, "EXPIRA: " .. vis.displayed_exp, 20, 20, 20, 255)
    draw_text(340, 320, "DEPTO: " .. vis.displayed_apt, 20, 20, 20, 255)

    -- Draw Entry Request Form
    draw_sprite("Paper.png", 660, 100, 460, 480)
    draw_text(700, 150, "SOLICITUD DE INGRESO D.D.D.", 20, 20, 20, 255)
    draw_text(700, 200, "INQUILINO: " .. vis.name .. " " .. vis.last_name, 40, 40, 40, 255)
    draw_text(700, 240, "APARTAMENTO DESTINO: " .. vis.displayed_apt, 40, 40, 40, 255)
    draw_text(700, 280, "OCUPACION: " .. vis.occupation, 40, 40, 40, 255)
    draw_text(700, 320, "RAZON: RETORNO DE TRABAJO", 40, 40, 40, 255)
    draw_text(700, 380, "SELLO D.D.D. VIGENTE: 1968", 180, 20, 20, 255)

    -- Help Instruction
    draw_rect(400, 600, 480, 35, 30, 30, 30, 255)
    draw_text(420, 610, "PULSA (O) PARA CERRAR INSPECCION DE DOCUMENTOS", 255, 255, 255, 255)
end

-- Render Today's List Overlay
local function render_list_inspection()
    draw_rect(200, 40, 880, 640, 10, 10, 15, 240)
    draw_rect(230, 60, 820, 600, 245, 240, 220, 255)

    draw_text(350, 90, "LISTA DE INGRESO AUTORIZADO - HOY", 20, 20, 20, 255)
    draw_rect(280, 120, 720, 2, 80, 80, 80, 255)

    local y_offset = 150
    for name, _ in pairs(game.todays_list) do
        draw_text(300, y_offset, "[X] " .. name, 30, 30, 30, 255)
        y_offset = y_offset + 35
    end

    draw_rect(400, 610, 480, 35, 30, 30, 30, 255)
    draw_text(440, 620, "PULSA (O) PARA CERRAR LISTA DE HOY", 255, 255, 255, 255)
end

-- Render Rotary Phone Dial Screen
local function render_phone_screen()
    draw_rect(300, 50, 680, 620, 25, 25, 30, 250)
    draw_sprite("Phone_back.png", 340, 80, 600, 560)
    draw_sprite("Phone_Roulette.png", 460, 220, 360, 360)

    draw_rect(420, 110, 440, 45, 10, 10, 10, 255)
    draw_text(435, 122, "NUMERO: " .. game.phone.dialed, 50, 255, 50, 255)

    if game.phone.response_text ~= "" then
        draw_rect(360, 170, 560, 35, 0, 0, 0, 220)
        draw_text(370, 180, game.phone.response_text, 255, 220, 50, 255)
    end

    -- Draw dial hitboxes
    for _, btn in pairs(game.phone_dial_buttons) do
        draw_rect(btn.x, btn.y, btn.w, btn.h, 40, 40, 40, 150)
        if btn.digit then
            draw_text(btn.x + 22, btn.y + 20, btn.digit, 255, 255, 255, 255)
        elseif btn.label then
            draw_text(btn.x + 10, btn.y + 15, btn.label, 255, 100, 100, 255)
        end
    end
end

-- Render Alarm Siren Screen
local function render_alarm_screen()
    draw_rect(0, 0, 1280, 720, 200, 0, 0, 120)
    draw_sprite("DDD_logo.png", 540, 200, 200, 200)
    draw_rect(200, 460, 880, 70, 0, 0, 0, 230)
    draw_text(260, 485, "ALARMA D.D.D. ACTIVADA - PROTOCOLO DE PURGA EN PROCESO", 255, 255, 0, 255)
end

-- Render Doppelgänger Attack Screen (Game Over)
local function render_attack_screen()
    draw_rect(0, 0, 1280, 720, 0, 0, 0, 255)
    draw_sprite("Doppel_Left_Hand.png", 100, 150, 300, 450)
    draw_sprite("Doppel_Right_Hand.png", 880, 150, 300, 450)

    draw_rect(240, 280, 800, 160, 50, 0, 0, 230)
    draw_text(360, 310, "¡HAS SIDO ASESINADO POR EL DOPPELGANGER!", 255, 50, 50, 255)
    draw_text(400, 360, "Permitiste la entrada a un ente peligroso.", 220, 220, 220, 255)
    draw_text(440, 400, "Pulsa (X) para reiniciar el turno.", 255, 255, 255, 255)
end

-- Render Victory Summary
local function render_victory_screen()
    draw_rect(150, 80, 980, 560, 20, 35, 20, 245)
    draw_sprite("DDD_logo.png", 560, 120, 160, 160)

    draw_text(440, 310, "EVALUACION DE TURNO D.D.D. - COMPLETO", 100, 255, 100, 255)
    draw_text(350, 370, "Residentes Legítimos Admitidos: " .. game.visitors_cleared, 255, 255, 255, 255)
    draw_text(350, 410, "Doppelgängers Neutralizados:    " .. game.doppels_eliminated, 255, 255, 255, 255)
    draw_text(350, 450, "Inocentes Eliminados por Error: " .. game.innocents_killed, 255, 100, 100, 255)

    draw_rect(420, 520, 440, 45, 0, 0, 0, 200)
    draw_text(460, 535, "PULSA (X) PARA INICIAR NUEVO TURNO", 255, 255, 0, 255)
end

-- Main Render Loop (called every frame from C)
function game_render()
    if game.state == STATES.DOPPEL_ATTACK then
        render_attack_screen()
    elseif game.state == STATES.SUMMARY_VICTORY then
        render_victory_screen()
    else
        render_booth()

        if game.state == STATES.INSPECT_DOC then
            render_doc_inspection()
        elseif game.state == STATES.INSPECT_LIST then
            render_list_inspection()
        elseif game.state == STATES.PHONE_CALL then
            render_phone_screen()
        end

        if game.alarm_active then
            render_alarm_screen()
        end

        -- Message Notification Banner at Top
        if game.banner_timer > 0 then
            draw_rect(0, 0, 1280, 42, 0, 0, 0, 220)
            draw_text(40, 14, game.message_banner, 255, 230, 80, 255)
        end
    end

    -- Always Render Virtual Cursor on top
    draw_sprite("Cursor.png", game.cursor.x - 16, game.cursor.y - 16, 32, 32)
end

function game_shutdown()
    log_info("Exiting That's Not My Neighbor PS3 Demake.")
end
