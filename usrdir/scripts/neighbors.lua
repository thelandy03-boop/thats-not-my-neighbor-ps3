-- That's Not My Neighbor - PS3 Demake
-- neighbors.lua: Tenants Registry, Doppelgänger Logic, and Anomalies

local neighbors = {}

-- Official Tenant Registry (lore-accurate based on DDD archives)
neighbors.database = {
    {
        key = "ROMAN_STILNSKY",
        name = "Roman",
        last_name = "Stilnsky",
        id_number = "483924059684",
        id_expiration = "05/1968",
        apartment = "F01-01",
        phone = "1152",
        occupation = "Contador",
        features = "Nariz grande, cicatriz en menton der., una sola ceja, usa sombrero.",
        head_asset = "Char_Roman_Head.png",
        body_asset = "Char_Roman_Body.png",
        hat_asset = "Char_Roman_Hat.png",
        eyes_asset = "Char_Roman_Eyes.png",
        nose_asset = "Char_Roman_Nose.png",
        mouth_asset = "Char_Roman_Mouth_01.png",
        scar_asset = "Char_Roman_Scar.png",
        photo_asset = "Char_Roman_Small_Photo.png"
    },
    {
        key = "FRANCIS_MOSSES",
        name = "Francis",
        last_name = "Mosses",
        id_number = "235569418995",
        id_expiration = "09/1964",
        apartment = "F03-02",
        phone = "4122",
        occupation = "Lechero",
        features = "Nariz larga, barbilla delgada, ojos cansados, usa sombrero blanco.",
        head_asset = "Char_Francis_Head.png",
        body_asset = "Char_Francis_Body.png",
        hat_asset = "Char_Francis_Hat.png",
        eyes_asset = "Char_Francis_Eyes.png",
        nose_asset = "Char_Francis_Nose.png",
        mouth_asset = "Char_Francis_Mouth_Doppel_01.png",
        photo_asset = "Char_Francis_Small_Photo.png"
    },
    {
        key = "LOIS_STILNSKY",
        name = "Lois",
        last_name = "Stilnsky",
        id_number = "894569723651",
        id_expiration = "10/1969",
        apartment = "F01-01",
        phone = "1152",
        occupation = "Ama de casa",
        features = "Labios grandes, lunar en mejilla izq., cabello corto.",
        photo_asset = "Frame_photo.png"
    },
    {
        key = "MARGARETTE_BUBBLES",
        name = "Margarette",
        last_name = "Bubbles",
        id_number = "103268014590",
        id_expiration = "12/1975",
        apartment = "F02-03",
        phone = "6431",
        occupation = "Costurera",
        features = "Cabello rizado, ojo izq. desviado, nariz redonda, sonrisa grande.",
        photo_asset = "Frame_photo.png"
    },
    {
        key = "ALF_CAPPUCCIN",
        name = "Alf",
        last_name = "Cappuccin",
        id_number = "120548096347",
        id_expiration = "08/1960",
        apartment = "F03-04",
        phone = "4258",
        occupation = "Abogado",
        features = "Nariz grande, bigote prominente, usa monoculo y sombrero.",
        photo_asset = "Frame_photo.png"
    },
    {
        key = "ANGUS_CIPRIANNI",
        name = "Angus",
        last_name = "Ciprianni",
        id_number = "565003698845",
        id_expiration = "05/1968",
        apartment = "F01-03",
        phone = "5513",
        occupation = "Empresario",
        features = "Cuello largo, usa sombrero, tiene bigote, ojos pequenos.",
        photo_asset = "Frame_photo.png"
    },
    {
        key = "MIA_STONE",
        name = "Mia",
        last_name = "Stone",
        id_number = "789564525115",
        id_expiration = "08/1978",
        apartment = "F03-01",
        phone = "1425",
        occupation = "Profesora",
        features = "Cabello largo, pecas, cara ovalada, cejas delgadas.",
        photo_asset = "Frame_photo.png"
    }
}

-- Doppelgänger Flaw Categories
neighbors.FLAW_TYPES = {
    WRONG_ID_NUMBER      = "El numero de ID no coincide con el archivo central",
    EXPIRED_ID           = "La tarjeta de identidad esta vencida",
    NOT_ON_TODAY_LIST    = "No figura en la lista de entradas autorizadas de hoy",
    WRONG_APARTMENT      = "Numero de apartamento falso o erroneo",
    FAKE_PHOTO           = "La foto del documento no corresponde con la persona",
    PHYSICAL_ANOMALY     = "Anomalia fisica grotesca detectable (ojo extra, sangre o deformidad)"
}

-- Generate Today's Authorized Entry List
function neighbors.generate_todays_list()
    local list = {}
    -- Always include Roman and Francis in today's baseline list
    list["Roman Stilnsky"] = true
    list["Francis Mosses"] = true
    list["Margarette Bubbles"] = true
    list["Angus Ciprianni"] = true
    return list
end

-- Spawn a visitor (can be a legitimate neighbor or a disguised doppelgänger)
function neighbors.spawn_visitor(force_doppel)
    local template_idx = math.random(1, 2) -- Focus on Roman and Francis who have full sprite assets
    local base = neighbors.database[template_idx]

    local visitor = {
        template = base,
        name = base.name,
        last_name = base.last_name,
        displayed_id = base.id_number,
        displayed_exp = base.id_expiration,
        displayed_apt = base.apartment,
        displayed_phone = base.phone,
        occupation = base.occupation,
        is_doppel = force_doppel or false,
        flaws = {},
        physical_extra_eye = false,
        physical_uzumaki = false,
        physical_blood = false,
        photo_tampered = false
    }

    if visitor.is_doppel then
        -- Select 1 to 3 anomalies
        local roll = math.random(1, 5)

        if roll == 1 then
            -- Mutate last 3 digits of ID
            local num = visitor.displayed_id
            visitor.displayed_id = string.sub(num, 1, #num - 3) .. tostring(math.random(100, 999))
            table.insert(visitor.flaws, neighbors.FLAW_TYPES.WRONG_ID_NUMBER)
        elseif roll == 2 then
            -- Expired document
            visitor.displayed_exp = "11/1952"
            table.insert(visitor.flaws, neighbors.FLAW_TYPES.EXPIRED_ID)
        elseif roll == 3 then
            -- Fake apartment
            visitor.displayed_apt = "F04-99"
            table.insert(visitor.flaws, neighbors.FLAW_TYPES.WRONG_APARTMENT)
        elseif roll == 4 then
            -- Physical grotesque mutation
            if base.key == "ROMAN_STILNSKY" then
                if math.random() > 0.5 then
                    visitor.physical_extra_eye = true
                else
                    visitor.physical_uzumaki = true
                end
            else
                visitor.physical_blood = true
            end
            table.insert(visitor.flaws, neighbors.FLAW_TYPES.PHYSICAL_ANOMALY)
        else
            -- Name not on today's list
            visitor.last_name = "Stilnsky-Imposter"
            table.insert(visitor.flaws, neighbors.FLAW_TYPES.NOT_ON_TODAY_LIST)
        end
    end

    return visitor
end

-- Validate visitor against central database and daily list
function neighbors.validate_visitor(visitor, todays_list)
    local errors = {}
    local full_name = visitor.name .. " " .. visitor.last_name

    if not todays_list[full_name] then
        table.insert(errors, neighbors.FLAW_TYPES.NOT_ON_TODAY_LIST)
    end

    if visitor.displayed_id ~= visitor.template.id_number then
        table.insert(errors, neighbors.FLAW_TYPES.WRONG_ID_NUMBER)
    end

    if visitor.displayed_apt ~= visitor.template.apartment then
        table.insert(errors, neighbors.FLAW_TYPES.WRONG_APARTMENT)
    end

    if visitor.displayed_exp < "01/1960" then
        table.insert(errors, neighbors.FLAW_TYPES.EXPIRED_ID)
    end

    if visitor.physical_extra_eye or visitor.physical_uzumaki or visitor.physical_blood then
        table.insert(errors, neighbors.FLAW_TYPES.PHYSICAL_ANOMALY)
    end

    local is_fake = (#errors > 0) or visitor.is_doppel
    return is_fake, errors
end

return neighbors
