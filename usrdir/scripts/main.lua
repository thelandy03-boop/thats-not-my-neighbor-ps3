-- That's Not My Neighbor - PS3 Demake
-- main.lua: Core Game Loop, State Controller, Interrogation, Title Screen, and Security Desk

local neighbors = require("neighbors")

-- Game States
local STATES = {
    TITLE             = 0, -- Title Screen & Main Menu
    WAIT_NEIGHBOR     = 1, -- Shutter closed, waiting for next tenant
    EXAMINE           = 2, -- Shutter open, neighbor outside, papers on desk
    INSPECT_DOC       = 3, -- Examining ID Card or Entry Request up close
    INSPECT_LIST      = 4, -- Inspecting Today's List
    PHONE_CALL        = 5, -- Rotary phone interface (DDD 3312 & tenant apartments)
    EMERGENCY_ALARM   = 6, -- Alarm siren, DDD cleaning protocol active
    DOPPEL_ATTACK     = 7, -- Player made a fatal mistake!
    SUMMARY_VICTORY   = 8, -- All visitors cleared successfully (Official DDD Grade)
    INSPECT_FLOORS    = 9, -- Building Tenants Directory Binder
    INSPECT_CHECKLIST = 10 -- Interactive DDD Verification Checklist Sheet
}

-- Global Game Context
local game = {
    running = true,
    state = STATES.TITLE,
    game_mode = "STANDARD",
    show_controls_modal = false,
    show_credits_modal = false,
    manual_page = 1,
    title_selected_idx = 1,
    title_anim_tick = 0,

    -- Virtual Cursor
    cursor = {
        x = 640.0,
        y = 360.0,
        speed = 12.0
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

    -- Dialogue & Interrogation System
    dialogue = {
        text = "",
        talking_timer = 0,
        mouth_frame_idx = 1,
        anim_tick = 0
    },

    -- Today's Approved Entry List
    todays_list = {},

    -- Building Floors Directory
    current_floor = 1,

    -- Interactive Verification Checklist (0 = empty, 1 = OK, 2 = DISCREPANCY/DOPPEL)
    checklist = {},

    -- Phone Context
    phone = {
        dialed = "",
        calling = false,
        response_text = ""
    },

    -- Title Screen Menu Items (DualShock 3 D-Pad + Virtual Cursor)
    title_menu = {
        { id = "standard",   x = 360, y = 295, w = 560, h = 48, label = "INICIAR TURNO ESTANDAR (5 VECINOS)", desc = "Turno regular de guardia. Evalua y coteja cada inquilino cuidadosamente." },
        { id = "extended",   x = 360, y = 350, w = 560, h = 48, label = "TURNO COMPLETO D.D.D. (10 VECINOS)", desc = "Jornada extendida con mayor afluencia y escrutinio riguroso del edificio." },
        { id = "nightmare",  x = 360, y = 405, w = 560, h = 48, label = "MODO PESADILLA // NIGHTMARE SHIFT",  desc = "ALERTA MAXIMA: Infiltracion masiva de entes grotescos, mutaciones y sangre." },
        { id = "manual",     x = 360, y = 460, w = 560, h = 48, label = "MANUAL Y PROTOCOLO D.D.D.",           desc = "Manual oficial con mapeo del mando DualShock 3 y directrices de purga." },
        { id = "credits",    x = 360, y = 515, w = 560, h = 48, label = "EXPEDIENTES Y CREDITOS D.D.D.",        desc = "Creditos al autor original Nacho Sama y arquitectura nativa PS3 Demake." },
        { id = "exit",       x = 360, y = 570, w = 560, h = 48, label = "SALIR AL MENU PRINCIPAL XMB",          desc = "Cierre seguro de sesion y regreso al sistema operativo CellOS." }
    },

    -- UI Hitboxes on Desk (X, Y, Width, Height)
    hitboxes = {
        shutter_toggle = { x = 320,  y = 220, w = 65,  h = 95  },
        door_unlock    = { x = 52,   y = 495, w = 106, h = 68  },
        door_lock      = { x = 52,   y = 580, w = 106, h = 68  },
        phone_desk     = { x = 180,  y = 480, w = 135, h = 190 },
        alarm_cover    = { x = 340,  y = 535, w = 65,  h = 130 },
        alarm_button   = { x = 345,  y = 550, w = 55,  h = 100 },
        desk_id        = { x = 440,  y = 485, w = 125, h = 80  },
        desk_request   = { x = 580,  y = 475, w = 115, h = 95  },
        desk_list      = { x = 730,  y = 475, w = 140, h = 190 },
        desk_floors    = { x = 895,  y = 465, w = 160, h = 205 },
        desk_checklist = { x = 1080, y = 470, w = 160, h = 200 }
    },

    -- Interrogation Question Buttons (available when window is open on left frame)
    interrogate_buttons = {
        reason = { x = 180, y = 140, w = 210, h = 42, label = "[?] MOTIVO VISITA" },
        id     = { x = 180, y = 195, w = 210, h = 42, label = "[?] CUESTIONAR ID" },
        list   = { x = 180, y = 250, w = 210, h = 42, label = "[?] NO EN LISTA" }
    },

    -- Directory Floor Tabs
    floor_tabs = {
        [1] = { x = 240, y = 80,  w = 140, h = 40, floor = 1, label = "PISO 1" },
        [2] = { x = 400, y = 80,  w = 140, h = 40, floor = 2, label = "PISO 2" },
        [3] = { x = 560, y = 80,  w = 140, h = 40, floor = 3, label = "PISO 3" }
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

-- Trigger Speech Dialogue with Mouth Animation
local function trigger_speech(text)
    if not text then return end
    game.dialogue.text = text
    game.dialogue.talking_timer = 240 -- ~4 seconds
    game.dialogue.mouth_frame_idx = 1
    game.dialogue.anim_tick = 0
    play_sfx("SFX_Select.wav")
end

-- Helper to Reset Verification Checklist
local function reset_checklist()
    game.checklist = {
        { label = "Foto de Identidad coincide con la persona", state = 0 },
        { label = "Numero de ID coincide con el archivo central", state = 0 },
        { label = "Fecha de caducidad vigente (>1960)", state = 0 },
        { label = "Apartamento y sello D.D.D. autorizados", state = 0 },
        { label = "Nombre en la lista de ingresos de hoy", state = 0 },
        { label = "Rasgos fisicos normales sin anomalias", state = 0 }
    }
end

-- Initialize Game
function game_init()
    log_info("Starting That's Not My Neighbor PS3 Demake...")
    game.todays_list = neighbors.generate_todays_list()
    game.state = STATES.TITLE
    game.game_mode = "STANDARD"
    game.show_controls_modal = false
    game.show_credits_modal = false
    game.manual_page = 1
    game.title_selected_idx = 1
    game.title_anim_tick = 0
    play_music("Music_Menu.ogg", true)

    reset_checklist()
    game.current_visitor = nil
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
    local is_nightmare = (game.game_mode == "NIGHTMARE")
    local force_doppel = nil
    if is_nightmare then
        force_doppel = (math.random() > 0.2)
    else
        force_doppel = (math.random() > 0.5)
    end

    game.current_visitor = neighbors.spawn_visitor(force_doppel, is_nightmare)
    game.shutter_open = false
    game.state = STATES.WAIT_NEIGHBOR
    game.dialogue.text = ""
    game.dialogue.talking_timer = 0
    reset_checklist()

    if is_nightmare then
        game.message_banner = "¡ALERTA ROJA! MODO PESADILLA ACTIVO: VERIFIQUE CADA ANOMALIA."
    else
        game.message_banner = "NUEVO INDIVIDUO EN LA PUERTA. ABRA LA VENTANILLA PARA EVALUAR."
    end
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

-- Handle Apartment Calling
local function handle_phone_dialed_number(number)
    play_sfx("SFX_Phone_Ring.ogg")

    if number == "3312" then
        game.phone.response_text = "D.D.D.: PROTOCOLO 3312 ACTIVO. SI DETECTO ANOMALIA, ACTIVE LA ALARMA."
        return
    end

    local found_apt = nil
    for floor_num = 1, 3 do
        for _, apt in ipairs(neighbors.floors[floor_num].apartments) do
            if apt.phone == number then
                found_apt = apt
                break
            end
        end
        if found_apt then break end
    end

    if not found_apt then
        game.phone.response_text = "NUMERO NO EXISTE O FUERA DE SERVICIO."
        return
    end

    if game.current_visitor and game.current_visitor.displayed_phone == number then
        if game.current_visitor.is_doppel then
            game.phone.response_text = string.format("DEPTO %s: '¿Hola? Estoy en casa... ¿Quien esta en la cabina?'", found_apt.apt)
        else
            game.phone.response_text = string.format("DEPTO %s: Suena el telefono pero nadie contesta...", found_apt.apt)
        end
    else
        game.phone.response_text = string.format("DEPTO %s (%s): '¿Si? ¿Quien habla? Estoy ocupado.'", found_apt.apt, found_apt.tenants)
    end
end

-- Main Update Loop (60 FPS)
function game_update()
    local pad = get_pad_input()

    -- 1. Virtual Cursor Movement (Mouse or Left Analog Stick with 15% Deadzone)
    if pad.mouse_moved and pad.mouse_x and pad.mouse_y then
        game.cursor.x = pad.mouse_x
        game.cursor.y = pad.mouse_y
    else
        local DEADZONE = 0.15
        local lx = pad.analog_lx or 0.0
        local ly = pad.analog_ly or 0.0
        if math.abs(lx) > DEADZONE then
            local mag = (math.abs(lx) - DEADZONE) / (1.0 - DEADZONE)
            local sign = lx > 0 and 1 or -1
            game.cursor.x = game.cursor.x + (sign * mag * game.cursor.speed)
        end
        if math.abs(ly) > DEADZONE then
            local mag = (math.abs(ly) - DEADZONE) / (1.0 - DEADZONE)
            local sign = ly > 0 and 1 or -1
            game.cursor.y = game.cursor.y + (sign * mag * game.cursor.speed)
        end
    end

    if game.cursor.x < 10.0  then game.cursor.x = 10.0 end
    if game.cursor.x > 1270.0 then game.cursor.x = 1270.0 end
    if game.cursor.y < 10.0  then game.cursor.y = 10.0 end
    if game.cursor.y > 710.0 then game.cursor.y = 710.0 end

    -- Detect Single Click Edge (Cross button) and Cancel (Circle button)
    local clicked = pad.cross and not game.pad_prev.cross
    local cancelled = pad.circle and not game.pad_prev.circle
    local l1_pressed = pad.l1 and not game.pad_prev.l1
    local r1_pressed = pad.r1 and not game.pad_prev.r1

    if game.banner_timer > 0 then
        game.banner_timer = game.banner_timer - 1
    end

    -- Mouth Animation Tick
    if game.dialogue.talking_timer > 0 then
        game.dialogue.talking_timer = game.dialogue.talking_timer - 1
        game.dialogue.anim_tick = game.dialogue.anim_tick + 1
        if game.dialogue.anim_tick % 7 == 0 and game.current_visitor and game.current_visitor.mouth_frames then
            local count = #game.current_visitor.mouth_frames
            if count > 0 then
                game.dialogue.mouth_frame_idx = (game.dialogue.mouth_frame_idx % count) + 1
            end
        end
    else
        game.dialogue.mouth_frame_idx = 1
    end

    -- 2. State-Specific Updates
    if game.state == STATES.TITLE then
        game.title_anim_tick = game.title_anim_tick + 1

        if game.show_controls_modal then
            -- Manual modal: L1 / R1 or D-Pad Left / Right changes page
            if l1_pressed or (pad.left and not game.pad_prev.left) then
                game.manual_page = (game.manual_page - 2) % 3 + 1
                play_sfx("SFX_Pick_Up_Folder.wav")
            elseif r1_pressed or (pad.right and not game.pad_prev.right) then
                game.manual_page = (game.manual_page % 3) + 1
                play_sfx("SFX_Pick_Up_Folder.wav")
            elseif clicked or cancelled then
                game.show_controls_modal = false
                play_sfx("SFX_Pick_Up_Folder.wav")
            end

        elseif game.show_credits_modal then
            -- Credits modal: Cross or Circle dismisses
            if clicked or cancelled then
                game.show_credits_modal = false
                play_sfx("SFX_Pick_Up_Folder.wav")
            end

        else
            -- D-Pad UP / DOWN navigation
            if pad.up and not game.pad_prev.up then
                game.title_selected_idx = game.title_selected_idx - 1
                if game.title_selected_idx < 1 then
                    game.title_selected_idx = #game.title_menu
                end
                play_sfx("SFX_Select.wav")
                local target = game.title_menu[game.title_selected_idx]
                game.cursor.x = target.x + target.w / 2
                game.cursor.y = target.y + target.h / 2
            elseif pad.down and not game.pad_prev.down then
                game.title_selected_idx = game.title_selected_idx + 1
                if game.title_selected_idx > #game.title_menu then
                    game.title_selected_idx = 1
                end
                play_sfx("SFX_Select.wav")
                local target = game.title_menu[game.title_selected_idx]
                game.cursor.x = target.x + target.w / 2
                game.cursor.y = target.y + target.h / 2
            end

            -- Virtual cursor hovering detection
            for i, btn in ipairs(game.title_menu) do
                if point_in_rect(game.cursor.x, game.cursor.y, btn) then
                    if game.title_selected_idx ~= i then
                        game.title_selected_idx = i
                        play_sfx("SFX_Select.wav")
                    end
                end
            end

            -- Action selection (Cross button)
            if clicked then
                local chosen = game.title_menu[game.title_selected_idx]
                if chosen then
                    if chosen.id == "standard" then
                        play_sfx("SFX_Accept.wav")
                        play_music("Music_Dark.ogg", true)
                        game.game_mode = "STANDARD"
                        game.visitor_queue = 5
                        game.visitors_cleared = 0
                        game.doppels_eliminated = 0
                        game.innocents_killed = 0
                        spawn_next()
                    elseif chosen.id == "extended" then
                        play_sfx("SFX_Accept.wav")
                        play_music("Music_Dark.ogg", true)
                        game.game_mode = "EXTENDED"
                        game.visitor_queue = 10
                        game.visitors_cleared = 0
                        game.doppels_eliminated = 0
                        game.innocents_killed = 0
                        spawn_next()
                    elseif chosen.id == "nightmare" then
                        play_sfx("SFX_Bite.wav")
                        play_music("Music_Dark.ogg", true)
                        game.game_mode = "NIGHTMARE"
                        game.visitor_queue = 8
                        game.visitors_cleared = 0
                        game.doppels_eliminated = 0
                        game.innocents_killed = 0
                        spawn_next()
                    elseif chosen.id == "manual" then
                        play_sfx("SFX_Pick_Up_Folder.wav")
                        game.show_controls_modal = true
                        game.manual_page = 1
                    elseif chosen.id == "credits" then
                        play_sfx("SFX_Pick_Up_Folder.wav")
                        game.show_credits_modal = true
                    elseif chosen.id == "exit" then
                        play_sfx("SFX_Hang_Down_Phone.wav")
                        exit_game()
                    end
                end
            end
        end

    elseif game.state == STATES.EMERGENCY_ALARM then
        game.alarm_timer = game.alarm_timer - 1
        if game.alarm_timer <= 0 then
            game.alarm_active = false
            spawn_next()
        end

    elseif game.state == STATES.DOPPEL_ATTACK then
        if clicked or cancelled then
            -- Return to Title Screen
            game.state = STATES.TITLE
            play_music("Music_Menu.ogg", true)
        end

    elseif game.state == STATES.SUMMARY_VICTORY then
        if clicked then
            -- Return to Title Screen after shift
            game.state = STATES.TITLE
            play_music("Music_Menu.ogg", true)
        end

    elseif game.state == STATES.INSPECT_DOC or game.state == STATES.INSPECT_LIST then
        local modal_rect = { x = 90, y = 40, w = 1100, h = 640 }
        if cancelled or (clicked and not point_in_rect(game.cursor.x, game.cursor.y, modal_rect)) then
            play_sfx("SFX_Pick_Up_Folder.wav")
            game.state = game.shutter_open and STATES.EXAMINE or STATES.WAIT_NEIGHBOR
        end

    elseif game.state == STATES.INSPECT_FLOORS then
        local modal_rect = { x = 100, y = 40, w = 1080, h = 640 }
        if l1_pressed then
            game.current_floor = math.max(1, game.current_floor - 1)
            play_sfx("SFX_Pick_Up_Folder.wav")
        elseif r1_pressed then
            game.current_floor = math.min(3, game.current_floor + 1)
            play_sfx("SFX_Pick_Up_Folder.wav")
        end

        if cancelled or (clicked and not point_in_rect(game.cursor.x, game.cursor.y, modal_rect)) then
            play_sfx("SFX_Pick_Up_Folder.wav")
            game.state = game.shutter_open and STATES.EXAMINE or STATES.WAIT_NEIGHBOR
        elseif clicked then
            for _, tab in ipairs(game.floor_tabs) do
                if point_in_rect(game.cursor.x, game.cursor.y, tab) then
                    game.current_floor = tab.floor
                    play_sfx("SFX_Pick_Up_Folder.wav")
                    break
                end
            end
        end

    elseif game.state == STATES.INSPECT_CHECKLIST then
        local modal_rect = { x = 140, y = 40, w = 1000, h = 640 }
        if cancelled or (clicked and not point_in_rect(game.cursor.x, game.cursor.y, modal_rect)) then
            play_sfx("SFX_Pick_Up_Folder.wav")
            game.state = game.shutter_open and STATES.EXAMINE or STATES.WAIT_NEIGHBOR
        elseif clicked then
            for i = 1, #game.checklist do
                local row_rect = { x = 200, y = 140 + (i * 65), w = 880, h = 50 }
                if point_in_rect(game.cursor.x, game.cursor.y, row_rect) then
                    game.checklist[i].state = (game.checklist[i].state + 1) % 3
                    play_sfx("SFX_Write.wav")
                    break
                end
            end
        end

    elseif game.state == STATES.PHONE_CALL then
        local phone_rect = { x = 300, y = 50, w = 680, h = 620 }
        if cancelled or (clicked and not point_in_rect(game.cursor.x, game.cursor.y, phone_rect)) then
            play_sfx("SFX_Hang_Down_Phone.wav")
            game.phone.dialed = ""
            game.phone.response_text = ""
            game.state = game.shutter_open and STATES.EXAMINE or STATES.WAIT_NEIGHBOR
        elseif clicked then
            for _, btn in pairs(game.phone_dial_buttons) do
                if point_in_rect(game.cursor.x, game.cursor.y, btn) then
                    if btn.digit then
                        if #game.phone.dialed < 4 then
                            game.phone.dialed = game.phone.dialed .. btn.digit
                            play_sfx("SFX_Dial_1.wav")

                            if #game.phone.dialed == 4 then
                                handle_phone_dialed_number(game.phone.dialed)
                            end
                        end
                    elseif btn.label == "Colgar" then
                        play_sfx("SFX_Hang_Down_Phone.wav")
                        game.phone.dialed = ""
                        game.phone.response_text = ""
                        game.state = game.shutter_open and STATES.EXAMINE or STATES.WAIT_NEIGHBOR
                    end
                end
            end
        end

    else
        -- Active security desk interaction
        if clicked then
            -- Interrogation Options (When window is open on left frame)
            if game.shutter_open and game.current_visitor and game.current_visitor.dialogues then
                if point_in_rect(game.cursor.x, game.cursor.y, game.interrogate_buttons.reason) then
                    trigger_speech(game.current_visitor.dialogues.reason)
                elseif point_in_rect(game.cursor.x, game.cursor.y, game.interrogate_buttons.id) then
                    trigger_speech(game.current_visitor.dialogues.id_inquiry)
                elseif point_in_rect(game.cursor.x, game.cursor.y, game.interrogate_buttons.list) then
                    trigger_speech(game.current_visitor.dialogues.list_inquiry)
                end
            end

            -- Toggle Shutter via Lever
            if point_in_rect(game.cursor.x, game.cursor.y, game.hitboxes.shutter_toggle) then
                game.shutter_open = not game.shutter_open
                play_sfx("SFX_Window.wav")
                if game.shutter_open then
                    game.state = STATES.EXAMINE
                    game.message_banner = "VENTANILLA ABIERTA. REVISE ROSTRO Y DOCUMENTOS EN MOSTRADOR."
                    if game.current_visitor and game.current_visitor.dialogues then
                        trigger_speech(game.current_visitor.dialogues.greeting)
                    end
                else
                    game.state = STATES.WAIT_NEIGHBOR
                    game.message_banner = "VENTANILLA CERRADA."
                    game.dialogue.talking_timer = 0
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

            -- Inspect ID Card (Delivered to counter when shutter is open)
            if game.shutter_open and game.current_visitor and point_in_rect(game.cursor.x, game.cursor.y, game.hitboxes.desk_id) then
                play_sfx("SFX_Pick_Up_Folder.wav")
                game.state = STATES.INSPECT_DOC
            end

            -- Inspect Entry Request (Delivered to counter when shutter is open)
            if game.shutter_open and game.current_visitor and point_in_rect(game.cursor.x, game.cursor.y, game.hitboxes.desk_request) then
                play_sfx("SFX_Pick_Up_Folder.wav")
                game.state = STATES.INSPECT_DOC
            end

            -- Inspect Today's List (Resting on right desk tray)
            if point_in_rect(game.cursor.x, game.cursor.y, game.hitboxes.desk_list) then
                play_sfx("SFX_Pick_Up_Folder.wav")
                game.state = STATES.INSPECT_LIST
            end

            -- Inspect Floors Directory Binder (Resting on right desk)
            if point_in_rect(game.cursor.x, game.cursor.y, game.hitboxes.desk_floors) then
                play_sfx("SFX_Pick_Up_Folder.wav")
                game.state = STATES.INSPECT_FLOORS
            end

            -- Inspect Verification Checklist (Resting on right desk)
            if point_in_rect(game.cursor.x, game.cursor.y, game.hitboxes.desk_checklist) then
                play_sfx("SFX_Pick_Up_Folder.wav")
                game.state = STATES.INSPECT_CHECKLIST
            end

            -- Open Rotary Phone
            if point_in_rect(game.cursor.x, game.cursor.y, game.hitboxes.phone_desk) then
                play_sfx("SFX_Pick_Up_Phone.wav")
                play_sfx("SFX_Dial_Tone.ogg")
                game.phone.dialed = ""
                game.phone.response_text = "MARQUE 3312 PARA D.D.D. O EL NUMERO DE UN DEPARTAMENTO"
                game.state = STATES.PHONE_CALL
            end

            -- Alarm Cover Toggle (Safety Flap)
            if point_in_rect(game.cursor.x, game.cursor.y, game.hitboxes.alarm_cover) then
                game.alarm_cover_open = not game.alarm_cover_open
                play_sfx("SFX_Select.wav")
            end

            -- Alarm Red Button (When Cover is Open)
            if game.alarm_cover_open and point_in_rect(game.cursor.x, game.cursor.y, game.hitboxes.alarm_button) then
                trigger_alarm()
            end
        end
    end

    -- Save previous pad state for single-click detection
    game.pad_prev = pad
end

-- Render Multi-Page Agent Handbook (Manual D.D.D.)
local function render_manual_modal()
    draw_rect(100, 45, 1080, 630, 10, 10, 15, 245)
    draw_rect(120, 65, 1040, 590, 240, 235, 215, 255)

    -- Header and D.D.D. Seal
    draw_sprite("DDD_logo.png", 140, 80, 100, 50)
    draw_text(260, 85, "DEPARTAMENTO DE DETECCION DE DOPPELGANGERS - MANUAL OFICIAL", 20, 20, 20, 255)
    draw_text(260, 110, "DOCUMENTO CLASIFICADO DE USO EXCLUSIVO PARA AGENTES EN CABINA", 140, 30, 30, 255)
    draw_rect(140, 140, 1000, 2, 70, 70, 70, 255)

    -- Tab Headers
    local tabs = {
        { id = 1, x = 160, label = "[1] MANDOS DUALSHOCK 3" },
        { id = 2, x = 480, label = "[2] PROTOCOLO DE SEGURIDAD" },
        { id = 3, x = 800, label = "[3] CATALOGO DE ANOMALIAS" }
    }

    for _, tab in ipairs(tabs) do
        if tab.id == game.manual_page then
            draw_rect(tab.x, 155, 280, 34, 40, 80, 40, 255)
            draw_text(tab.x + 20, 164, tab.label, 255, 255, 255, 255)
        else
            draw_rect(tab.x, 155, 280, 34, 180, 175, 160, 255)
            draw_text(tab.x + 20, 164, tab.label, 60, 60, 60, 255)
        end
    end

    -- Tab 1: Mando DualShock 3
    if game.manual_page == 1 then
        draw_rect(160, 210, 960, 380, 250, 246, 230, 255)
        draw_text(180, 230, "MAPEO Y DISTRIBUCION DEL CONTROLADOR PS3:", 20, 20, 20, 255)

        draw_text(200, 270, "* STICK ANALOGICO IZQUIERDO / CRUCETA (D-PAD):", 40, 40, 40, 255)
        draw_text(220, 295, "  Mueve el cursor virtual por el escritorio, documentos y ventanilla.", 80, 80, 80, 255)

        draw_text(200, 335, "* BOTON CRUZ (X):", 40, 40, 40, 255)
        draw_text(220, 360, "  Interactuar con objetos, abrir/cerrar persiana, sellar documentos y marcar casillas.", 80, 80, 80, 255)

        draw_text(200, 400, "* BOTON CIRCULO (O):", 40, 40, 40, 255)
        draw_text(220, 425, "  Cerrar cualquier documento o carpeta abierta; colgar el telefono de disco.", 80, 80, 80, 255)

        draw_text(200, 465, "* BOTONES L1 / R1:", 40, 40, 40, 255)
        draw_text(220, 490, "  Navegar entre pisos en la Carpeta del Edificio o cambiar de pagina en manuales.", 80, 80, 80, 255)

        draw_text(200, 530, "* BOTON ROJO DE ALARMA:", 160, 30, 30, 255)
        draw_text(220, 555, "  Levante la tapa de seguridad primero antes de presionar el boton de purga D.D.D.", 80, 80, 80, 255)

    -- Tab 2: Protocolo Operativo D.D.D.
    elseif game.manual_page == 2 then
        draw_rect(160, 210, 960, 380, 250, 246, 230, 255)
        draw_text(180, 230, "PROCEDIMIENTO ESTANDAR ANTE LA LLEGADA DE UN VECINO:", 20, 20, 20, 255)

        draw_text(200, 270, "1. ACCIONAR PALANCA DE VENTANILLA:", 30, 30, 30, 255)
        draw_text(220, 295, "   Reciba al vecino y observe atentamente sus expresiones faciales y atuendo.", 70, 70, 70, 255)

        draw_text(200, 335, "2. SOLICITAR IDENTIFICACION Y PERMISO:", 30, 30, 30, 255)
        draw_text(220, 360, "   Haga clic sobre el Carnet de Identidad y la Solicitud para abrirlos.", 70, 70, 70, 255)

        draw_text(200, 400, "3. COTEJAR REGISTROS:", 30, 30, 30, 255)
        draw_text(220, 425, "   Verifique que figure en la Lista de Hoy y que el Depto coincida con la Carpeta de Pisos.", 70, 70, 70, 255)

        draw_text(200, 465, "4. REGISTRO EN HOJA DE COTEJO:", 30, 30, 30, 255)
        draw_text(220, 490, "   Marque cada punto validado. Un cotejo completo garantiza un turno con calificacion S.", 70, 70, 70, 255)

        draw_text(200, 530, "5. RESOLUCION FINAL:", 30, 30, 30, 255)
        draw_text(220, 555, "   Si todo es correcto: Boton Verde. Si detecta impostor: Levante la tapa y active Alarma.", 150, 20, 20, 255)

    -- Tab 3: Guía de Anomalías
    elseif game.manual_page == 3 then
        draw_rect(160, 210, 960, 380, 250, 246, 230, 255)
        draw_text(180, 230, "CLASIFICACION DE ANOMALIAS DE DOPPELGANGERS:", 180, 20, 20, 255)

        draw_text(200, 270, "* FALSIFICACIONES DOCUMENTALES:", 30, 30, 30, 255)
        draw_text(220, 295, "  Numeros de ID con digitos faltantes, sellos DDD falsos o fechas de caducidad expiradas.", 70, 70, 70, 255)

        draw_text(200, 335, "* ANOMALIAS FISICAS GROTESCAS:", 30, 30, 30, 255)
        draw_text(220, 360, "  Tercer ojo, cara con espiral uzumaki, manchas de sangre, miembros duplicados u orejas mutadas.", 70, 70, 70, 255)

        draw_text(200, 400, "* AUSENCIA EN LISTA OFICIAL:", 30, 30, 30, 255)
        draw_text(220, 425, "  Nombres alterados que no aparecen en la lista de ingreso diario autorizada.", 70, 70, 70, 255)

        draw_text(200, 465, "* LLAMADA DE VERIFICACION:", 30, 30, 30, 255)
        draw_text(220, 490, "  Si marca al departamento y el residente original contesta, quien esta afuera es un Doppelgänger.", 160, 30, 30, 255)

        draw_text(200, 530, "* PROTOCOLO DE PURGA 3312:", 30, 30, 30, 255)
        draw_text(220, 555, "  La unidad DDD neutralizara inmediatamente a cualquier entidad anomala reportada.", 60, 60, 60, 255)
    end

    -- Footer
    draw_rect(240, 605, 800, 36, 30, 30, 35, 230)
    draw_text(290, 615, "L1 / R1: CAMBIAR PAGINA   |   PULSA (O) O (X) PARA CERRAR MANUAL", 255, 255, 255, 255)
end

-- Render Credits & Technical Demake Dossier
local function render_credits_modal()
    draw_rect(140, 60, 1000, 600, 10, 10, 15, 245)
    draw_rect(160, 80, 960, 560, 240, 235, 215, 255)

    -- Header and D.D.D. Seal
    draw_sprite("DDD_logo.png", 540, 95, 200, 90)
    draw_text(410, 200, "THAT'S NOT MY NEIGHBOR", 20, 20, 20, 255)
    draw_text(370, 225, "EXPEDIENTE HISTORICO Y CREDITOS DEL DEMAKE", 160, 30, 30, 255)
    draw_rect(200, 255, 880, 2, 80, 80, 80, 255)

    -- Original Game Credits
    draw_text(220, 280, "JUEGO ORIGINAL:", 20, 20, 20, 255)
    draw_text(240, 305, "* Creado, programado y disenado por: Nacho Sama", 40, 40, 40, 255)
    draw_text(240, 330, "* Todos los derechos de arte, personajes, musica y universo pertenecen a Nacho Sama.", 70, 70, 70, 255)

    -- PS3 Demake Credits
    draw_text(220, 375, "PLAYSTATION 3 HOMEBREW DEMAKE:", 20, 20, 20, 255)
    draw_text(240, 400, "* Arquitectura: PSL1GHT v2 ANSI C nativo para PPU (Cell OS)", 40, 40, 40, 255)
    draw_text(240, 425, "* Motor Grafico: RSX 720p Tiny3D con mapeo estatico VRAM (256MB GDDR3)", 40, 40, 40, 255)
    draw_text(240, 450, "* Motor de Script: Vanilla Lua 5.4.6 embebido (sin JIT por seguridad W^X)", 40, 40, 40, 255)
    draw_text(240, 475, "* Sin dependencias de motores pesados: 100% optimizado para hardware de PS3", 40, 40, 40, 255)

    -- Dedication
    draw_text(220, 520, "DEDICATORIA:", 20, 20, 20, 255)
    draw_text(240, 545, "A la comunidad homebrew y de preservacion de consolas de septima generacion.", 70, 70, 70, 255)

    -- Footer
    draw_rect(340, 585, 600, 36, 30, 30, 35, 230)
    draw_text(430, 595, "PULSA (O) O (X) PARA CERRAR EXPEDIENTE", 255, 255, 255, 255)
end

-- Render Title Screen & Main Menu
local function render_title_screen()
    -- Atmosphere Dark Wall Base
    draw_rect(0, 0, 1280, 720, 10, 8, 14, 255)
    draw_sprite("Wall.png", 0, 0, 1280, 720)
    draw_rect(0, 0, 1280, 720, 0, 0, 0, 185)

    -- Subtle CRT Scanlines
    local scan_y = (game.title_anim_tick * 2.5) % 720
    draw_rect(0, scan_y, 1280, 3, 255, 255, 255, 16)

    -- Lurking Doppelgänger hands swaying at bottom corners
    local sway = math.sin(game.title_anim_tick * 0.05) * 6
    draw_sprite("Doppel_Left_Hand.png", 20, 440 + sway, 200, 270)
    draw_sprite("Doppel_Right_Hand.png", 1060, 440 - sway, 200, 270)

    -- Official Banner & D.D.D. Logo Frame
    draw_rect(350, 15, 580, 260, 15, 15, 20, 215)
    draw_rect(350, 15, 580, 2, 70, 60, 40, 255)
    draw_rect(350, 273, 580, 2, 70, 60, 40, 255)

    draw_sprite("DDD_logo.png", 540, 22, 200, 95)
    draw_sprite("banner.png", 490, 115, 300, 120)

    -- Title Typography
    draw_text(420, 238, "DEPARTAMENTO DE DETECCION DE DOPPELGANGERS", 220, 50, 50, 255)
    draw_text(475, 256, "[ EXPEDIENTE DE SEGURIDAD 1955-D ]", 255, 220, 100, 255)

    -- Interactive Buttons
    for i, btn in ipairs(game.title_menu) do
        local is_selected = (i == game.title_selected_idx)
        local is_nightmare_btn = (btn.id == "nightmare")

        if is_selected then
            if is_nightmare_btn then
                draw_rect(btn.x - 4, btn.y - 4, btn.w + 8, btn.h + 8, 255, 40, 40, 255)
                draw_rect(btn.x, btn.y, btn.w, btn.h, 70, 20, 20, 255)
                draw_text(btn.x + 25, btn.y + 16, "> " .. btn.label, 255, 200, 200, 255)
            else
                draw_rect(btn.x - 4, btn.y - 4, btn.w + 8, btn.h + 8, 255, 215, 75, 255)
                draw_rect(btn.x, btn.y, btn.w, btn.h, 55, 45, 35, 255)
                draw_text(btn.x + 25, btn.y + 16, "> " .. btn.label, 255, 240, 120, 255)
            end
        else
            if is_nightmare_btn then
                draw_rect(btn.x, btn.y, btn.w, btn.h, 40, 15, 15, 230)
                draw_rect(btn.x + 2, btn.y + 2, btn.w - 4, btn.h - 4, 30, 10, 10, 240)
                draw_text(btn.x + 25, btn.y + 16, "  " .. btn.label, 200, 100, 100, 255)
            else
                draw_rect(btn.x, btn.y, btn.w, btn.h, 30, 26, 26, 230)
                draw_rect(btn.x + 2, btn.y + 2, btn.w - 4, btn.h - 4, 20, 18, 18, 240)
                draw_text(btn.x + 25, btn.y + 16, "  " .. btn.label, 220, 220, 220, 255)
            end
        end
    end

    -- Description Banner for Selected Button
    local active_btn = game.title_menu[game.title_selected_idx]
    if active_btn and active_btn.desc then
        draw_rect(320, 630, 640, 32, 12, 12, 16, 230)
        draw_rect(320, 630, 640, 1, 70, 70, 80, 255)
        draw_text(340, 638, active_btn.desc, 255, 240, 160, 255)
    end

    -- Bottom Controls Bar
    draw_text(260, 680, "[ D-PAD / STICK: NAVEGAR ]   [ X: CONFIRMAR ]   [ O: CANCELAR ]   [ CELL/RSX 720p 60FPS ]", 150, 150, 150, 255)

    -- Render Modals on Top if Open
    if game.show_controls_modal then
        render_manual_modal()
    elseif game.show_credits_modal then
        render_credits_modal()
    end
end

-- Render Security Booth Background & Elements
local function render_booth()
    draw_sprite("Background.png", 0, 0, 1280, 720)

    -- If shutter is open, draw the visitor outside the window
    if game.shutter_open and game.current_visitor then
        local vis = game.current_visitor
        local tx = 520
        local ty = 140

        -- Character Body
        if vis.template.body_asset then
            if vis.physical_blood then
                draw_sprite("Char_Francis_Body_Blood.png", tx, ty + 120, 240, 260)
            else
                draw_sprite(vis.template.body_asset, tx, ty + 120, 240, 260)
            end
        end

        -- Extra arms for Roman doppelgänger
        if vis.physical_extra_arms then
            draw_sprite("Char_Roman_Doppel_Extra_Arms.png", tx - 30, ty + 100, 300, 260)
        end

        -- Character Head
        if vis.template.head_asset then
            if vis.physical_blood and vis.template.key == "FRANCIS_MOSSES" then
                draw_sprite("Char_Francis_Head_Blood.png", tx + 20, ty, 200, 190)
            else
                draw_sprite(vis.template.head_asset, tx + 20, ty, 200, 190)
            end
        end

        -- Ears for Francis (when doppelgänger)
        if vis.flaws_has_ears then
            draw_sprite("Char_Francis_Ears_Doppel.png", tx + 10, ty + 40, 220, 70)
        end

        -- Eyebrows
        if vis.flaws_has_eyebrows then
            draw_sprite("Char_Francis_Eyebrows_Doppel.png", tx + 35, ty + 40, 170, 30)
        elseif vis.template.eyebrows_asset then
            draw_sprite(vis.template.eyebrows_asset, tx + 35, ty + 40, 170, 30)
        end

        -- Eyes
        if vis.flaws_has_eyes then
            draw_sprite("Char_Francis_Eyes_Doppel.png", tx + 40, ty + 50, 160, 40)
        elseif vis.template.eyes_asset then
            draw_sprite(vis.template.eyes_asset, tx + 40, ty + 50, 160, 40)
        end

        -- Nose
        if vis.flaws_has_nose then
            draw_sprite("Char_Francis_Nose_Doppel.png", tx + 70, ty + 65, 60, 65)
        elseif vis.template.nose_asset then
            draw_sprite(vis.template.nose_asset, tx + 70, ty + 70, 60, 60)
        end

        -- Scar (Roman)
        if vis.template.scar_asset then
            draw_sprite(vis.template.scar_asset, tx + 70, ty + 115, 45, 30)
        end

        -- Animated Mouth Flap Sync
        local mouth_sprite = vis.template.mouth_asset
        if vis.mouth_frames and #vis.mouth_frames > 0 then
            if game.dialogue.talking_timer > 0 then
                mouth_sprite = vis.mouth_frames[game.dialogue.mouth_frame_idx] or vis.mouth_frames[1]
            else
                mouth_sprite = vis.mouth_frames[1]
            end
        end
        if mouth_sprite then
            draw_sprite(mouth_sprite, tx + 60, ty + 130, 80, 40)
        end

        -- Hat
        if vis.template.hat_asset then
            if vis.physical_blood and vis.template.key == "FRANCIS_MOSSES" then
                draw_sprite("Char_Francis_Hat_Blood.png", tx + 10, ty - 50, 220, 110)
            else
                draw_sprite(vis.template.hat_asset, tx + 10, ty - 50, 220, 110)
            end
        end

        -- Anomalous physical features (Roman)
        if vis.physical_extra_eye then
            draw_sprite("Char_Roman_Doppel_3rd_eye.png", tx + 75, ty + 20, 50, 40)
        end
        if vis.physical_uzumaki then
            draw_sprite("Char_Roman_Doppel_Uzumaki_Face.png", tx + 20, ty, 200, 190)
        end

        -- Speech Balloon Overlay above window
        if game.dialogue.talking_timer > 0 and game.dialogue.text ~= "" then
            draw_rect(340, 35, 600, 50, 10, 10, 15, 235)
            draw_text(355, 52, game.dialogue.text, 255, 240, 150, 255)
        end

        -- Interrogation Options Panel on Left
        for _, btn in pairs(game.interrogate_buttons) do
            draw_rect(btn.x, btn.y, btn.w, btn.h, 25, 25, 30, 210)
            draw_rect(btn.x + 2, btn.y + 2, btn.w - 4, btn.h - 4, 45, 45, 55, 230)
            draw_text(btn.x + 15, btn.y + 15, btn.label, 255, 255, 255, 255)
        end
    end

    -- Booth Wall, Hatch frame, and Window
    draw_sprite("Wall.png", 0, 0, 1280, 720)
    if not game.shutter_open then
        draw_sprite("Panic_window.png", 420, 85, 440, 390)
    end

    -- Shutter Control Switch / Lever (Mounted diegetically on left window frame)
    draw_sprite("Entry_request_button.png", 320, 220, 65, 95)
    if game.shutter_open then
        draw_rect(348, 230, 9, 9, 50, 220, 50, 255) -- Green status LED
    else
        draw_rect(348, 230, 9, 9, 220, 50, 50, 255) -- Red status LED
    end

    -- Door Control Console on Left Desk (Integrated unlock/lock buttons)
    draw_sprite("Door_control.png", 40, 460, 130, 230)
    draw_sprite("Door_button_unlock.png", 52, 495, 106, 68)
    draw_sprite("Door_button_lock.png", 52, 580, 106, 68)

    -- Rotary Phone on Lower-Left Desk
    draw_sprite("Phone_base.png", 180, 480, 135, 190)
    draw_sprite("Phone_top.png", 182, 475, 130, 70)

    -- D.D.D. Emergency Alarm Box (Integrated Console, ZERO debug text)
    draw_rect(335, 530, 75, 140, 45, 42, 42, 255)
    draw_rect(339, 534, 67, 132, 20, 18, 18, 255)

    if game.alarm_cover_open then
        -- Open safety flap resting upwards
        draw_rect(340, 500, 65, 30, 210, 175, 30, 255)
        -- Exposed Red Plunger Button
        draw_rect(345, 550, 55, 100, 220, 30, 30, 255)
        draw_rect(348, 553, 49, 94, 255, 60, 60, 255)
    else
        -- Closed Hazard Yellow Safety Flap with Black Diagonal Accent Bands
        draw_rect(340, 535, 65, 130, 210, 175, 30, 255)
        draw_rect(340, 555, 65, 14, 20, 20, 20, 255)
        draw_rect(340, 595, 65, 14, 20, 20, 20, 255)
        draw_rect(340, 635, 65, 14, 20, 20, 20, 255)
    end

    -- Visitor Documents (Delivered through the hatch slot onto center-left counter ONLY when open)
    if game.shutter_open and game.current_visitor then
        draw_sprite("ID.png", 440, 485, 125, 80)
        draw_sprite("Paper.png", 580, 475, 115, 95)
    end

    -- Reference Documents on Right Desk (Resting naturally, ZERO debug text labels)
    draw_sprite("Todays_list.png", 730, 475, 140, 190)
    draw_sprite("Folder_floors.png", 895, 465, 160, 205)
    draw_sprite("Papers.png", 1080, 470, 160, 200)

    -- DDD Security Plaque on Upper Wall
    draw_sprite("DDD_logo.png", 1120, 25, 130, 65)
end

-- Render Document Inspection Overlay
local function render_doc_inspection()
    draw_rect(100, 50, 1080, 620, 10, 10, 15, 240)
    draw_rect(120, 70, 1040, 580, 240, 230, 200, 255)

    local vis = game.current_visitor
    if not vis then return end

    -- Draw ID Card Inspection
    draw_sprite("ID.png", 160, 120, 460, 320)
    local photo_to_draw = vis.photo_asset or vis.template.photo_asset
    if photo_to_draw then
        draw_sprite(photo_to_draw, 190, 170, 130, 160)
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

-- Render Building Tenants Directory (Carpeta de Pisos)
local function render_floors_directory()
    draw_rect(100, 40, 1080, 640, 15, 15, 20, 240)
    draw_sprite("Folder_floors.png", 140, 60, 1000, 600)

    for _, tab in ipairs(game.floor_tabs) do
        if tab.floor == game.current_floor then
            draw_rect(tab.x, tab.y, tab.w, tab.h, 220, 200, 160, 255)
            draw_text(tab.x + 35, tab.y + 12, tab.label, 0, 0, 0, 255)
        else
            draw_rect(tab.x, tab.y, tab.w, tab.h, 80, 60, 40, 230)
            draw_text(tab.x + 35, tab.y + 12, tab.label, 200, 200, 200, 255)
        end
    end

    local floor_data = neighbors.floors[game.current_floor]
    if floor_data then
        local y_pos = 150
        for _, apt in ipairs(floor_data.apartments) do
            draw_rect(220, y_pos, 840, 100, 245, 240, 225, 255)
            draw_sprite(apt.photo, 230, y_pos + 10, 70, 80)

            draw_text(320, y_pos + 15, "DEPTO: " .. apt.apt .. "  |  TELEFONO: " .. apt.phone, 20, 20, 20, 255)
            draw_text(320, y_pos + 45, "INQUILINOS: " .. apt.tenants, 40, 40, 40, 255)
            draw_text(320, y_pos + 70, "NOTAS D.D.D.: " .. apt.note, 80, 80, 80, 255)

            y_pos = y_pos + 115
        end
    end

    draw_rect(360, 620, 560, 30, 20, 20, 20, 220)
    draw_text(380, 628, "L1/R1 CAMBIA PISO | PULSA (O) PARA CERRAR DIRECTORIO", 255, 255, 255, 255)
end

-- Render Interactive Verification Checklist (Hoja de Cotejo)
local function render_checklist_overlay()
    draw_rect(140, 40, 1000, 640, 10, 10, 15, 245)
    draw_rect(170, 60, 940, 600, 245, 240, 225, 255)

    draw_text(380, 85, "HOJA DE VERIFICACION D.D.D. - PROTOCOLO DE COTEJO", 20, 20, 20, 255)
    draw_rect(200, 115, 880, 2, 60, 60, 60, 255)

    for i, item in ipairs(game.checklist) do
        local box_y = 140 + (i * 65)
        draw_rect(220, box_y, 45, 45, 40, 40, 40, 255)
        draw_rect(224, box_y + 4, 37, 37, 255, 255, 255, 255)

        if item.state == 1 then
            draw_rect(228, box_y + 8, 29, 29, 40, 180, 40, 255)
            draw_text(236, box_y + 14, "V", 255, 255, 255, 255)
        elseif item.state == 2 then
            draw_rect(228, box_y + 8, 29, 29, 200, 30, 30, 255)
            draw_text(236, box_y + 14, "X", 255, 255, 255, 255)
        end

        draw_text(285, box_y + 14, item.label, 30, 30, 30, 255)

        if item.state == 2 then
            draw_text(850, box_y + 14, "[DISCREPANCIA]", 200, 20, 20, 255)
        elseif item.state == 1 then
            draw_text(850, box_y + 14, "[CONFORME]", 20, 140, 20, 255)
        end
    end

    draw_rect(280, 615, 720, 35, 20, 20, 20, 230)
    draw_text(300, 625, "HAZ CLIC EN CADA FILA PARA MARCAR | PULSA (O) PARA CERRAR CHECKLIST", 255, 255, 255, 255)
end

-- Render Rotary Phone Dial Screen
local function render_phone_screen()
    draw_rect(300, 50, 680, 620, 25, 25, 30, 250)
    draw_sprite("Phone_back.png", 340, 80, 600, 560)
    draw_sprite("Phone_Roulette.png", 460, 220, 360, 360)

    draw_rect(420, 110, 440, 45, 10, 10, 10, 255)
    draw_text(435, 122, "NUMERO: " .. game.phone.dialed, 50, 255, 50, 255)

    if game.phone.response_text ~= "" then
        draw_rect(340, 165, 600, 40, 0, 0, 0, 220)
        draw_text(350, 175, game.phone.response_text, 255, 220, 50, 255)
    end

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
    draw_text(440, 400, "Pulsa (X) para volver al Menu Principal.", 255, 255, 255, 255)
end

-- Render Official DDD Clearance Certificate & Letter Grade
local function render_victory_screen()
    draw_rect(100, 50, 1080, 620, 15, 20, 15, 245)
    draw_rect(130, 70, 1020, 580, 245, 240, 220, 255)

    draw_sprite("DDD_logo.png", 560, 90, 160, 80)
    draw_text(320, 185, "DEPARTAMENTO DE DETECCION DE DOPPELGANGERS (D.D.D.)", 20, 20, 20, 255)
    draw_text(360, 215, "CERTIFICADO OFICIAL DE EVALUACION DE TURNO", 40, 40, 40, 255)
    draw_rect(200, 245, 880, 2, 80, 80, 80, 255)

    -- Metrics Breakdown
    draw_text(260, 280, "Residentes Legítimos Admitidos:     " .. game.visitors_cleared, 30, 30, 30, 255)
    draw_text(260, 325, "Doppelgängers Neutralizados:        " .. game.doppels_eliminated, 30, 140, 30, 255)
    draw_text(260, 370, "Inocentes Eliminados por Error:     " .. game.innocents_killed, 200, 30, 30, 255)

    -- Rank Calculation
    local rank_letter = "S"
    local rank_text = "EXCELENCIA - TURNO PERFECTO"
    local rank_color_r = 30
    local rank_color_g = 180
    local rank_color_b = 30

    if game.innocents_killed == 1 then
        rank_letter = "B"
        rank_text = "APROBADO CON OBSERVACIONES"
        rank_color_r = 200
        rank_color_g = 160
        rank_color_b = 30
    elseif game.innocents_killed >= 2 then
        rank_letter = "F"
        rank_text = "TERMINACION INMEDIATA DE CONTRATO"
        rank_color_r = 220
        rank_color_g = 20
        rank_color_b = 20
    end

    -- Stamp Seal Box
    draw_rect(760, 270, 240, 140, rank_color_r, rank_color_g, rank_color_b, 255)
    draw_rect(765, 275, 230, 130, 245, 240, 220, 255)
    draw_text(810, 300, "RANGO: " .. rank_letter, rank_color_r, rank_color_g, rank_color_b, 255)
    draw_text(775, 360, rank_text, rank_color_r, rank_color_g, rank_color_b, 255)

    draw_rect(400, 560, 480, 45, 20, 20, 20, 230)
    draw_text(430, 575, "PULSA (X) PARA VOLVER AL MENU PRINCIPAL", 255, 255, 0, 255)
end

-- Helper to detect if cursor is hovering over an interactive object
local function is_cursor_interactive()
    local cx, cy = game.cursor.x, game.cursor.y
    if game.state == STATES.TITLE then
        if game.show_controls_modal or game.show_credits_modal then return true end
        for _, btn in ipairs(game.title_menu) do
            if point_in_rect(cx, cy, btn) then return true end
        end
        return false
    end

    if game.state == STATES.WAIT_NEIGHBOR or game.state == STATES.EXAMINE then
        if point_in_rect(cx, cy, game.hitboxes.shutter_toggle) then return true end
        if point_in_rect(cx, cy, game.hitboxes.door_unlock) then return true end
        if point_in_rect(cx, cy, game.hitboxes.door_lock) then return true end
        if point_in_rect(cx, cy, game.hitboxes.phone_desk) then return true end
        if point_in_rect(cx, cy, game.hitboxes.alarm_cover) then return true end
        if game.alarm_cover_open and point_in_rect(cx, cy, game.hitboxes.alarm_button) then return true end

        if game.shutter_open and game.current_visitor then
            if point_in_rect(cx, cy, game.hitboxes.desk_id) then return true end
            if point_in_rect(cx, cy, game.hitboxes.desk_request) then return true end
            for _, btn in pairs(game.interrogate_buttons) do
                if point_in_rect(cx, cy, btn) then return true end
            end
        end

        if point_in_rect(cx, cy, game.hitboxes.desk_list) then return true end
        if point_in_rect(cx, cy, game.hitboxes.desk_floors) then return true end
        if point_in_rect(cx, cy, game.hitboxes.desk_checklist) then return true end
    elseif game.state == STATES.PHONE_CALL then
        for _, btn in pairs(game.phone_dial_buttons) do
            if point_in_rect(cx, cy, btn) then return true end
        end
    elseif game.state == STATES.INSPECT_FLOORS then
        for _, tab in ipairs(game.floor_tabs) do
            if point_in_rect(cx, cy, tab) then return true end
        end
    end
    return false
end

-- Main Render Loop (called every frame from C)
function game_render()
    if game.state == STATES.TITLE then
        render_title_screen()
    elseif game.state == STATES.DOPPEL_ATTACK then
        render_attack_screen()
    elseif game.state == STATES.SUMMARY_VICTORY then
        render_victory_screen()
    else
        render_booth()

        if game.state == STATES.INSPECT_DOC then
            render_doc_inspection()
        elseif game.state == STATES.INSPECT_LIST then
            render_list_inspection()
        elseif game.state == STATES.INSPECT_FLOORS then
            render_floors_directory()
        elseif game.state == STATES.INSPECT_CHECKLIST then
            render_checklist_overlay()
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

    -- Discrete Virtual Cursor (Hotspot at pointer tip)
    local hovering = is_cursor_interactive()
    if hovering then
        draw_rect(game.cursor.x - 2, game.cursor.y - 2, 7, 7, 255, 220, 80, 220)
        draw_sprite("Cursor.png", game.cursor.x - 2, game.cursor.y - 2, 34, 34)
    else
        draw_sprite("Cursor.png", game.cursor.x - 2, game.cursor.y - 2, 28, 28)
    end
end

function game_shutdown()
    log_info("Exiting That's Not My Neighbor PS3 Demake.")
end
