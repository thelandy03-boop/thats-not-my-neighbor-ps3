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
        eyebrows_asset = "Char_Francis_Eyebrows.png",
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
    },
    {
        key = "ROBERTSKY_PEACHMAN",
        name = "Robertsky",
        last_name = "Peachman",
        id_number = "114652289730",
        id_expiration = "12/1973",
        apartment = "F01-02",
        phone = "2668",
        occupation = "Zapatero",
        features = "Cuello largo, sin cejas, nariz grande, barba de chivo.",
        photo_asset = "Frame_photo.png"
    },
    {
        key = "ARNOLD_SCHMICHT",
        name = "Arnold",
        last_name = "Schmicht",
        id_number = "778566219322",
        id_expiration = "10/1971",
        apartment = "F02-01",
        phone = "5123",
        occupation = "Escritor",
        features = "Cejas grandes, bigote prominente, sombrero, cara redonda.",
        photo_asset = "Frame_photo.png"
    },
    {
        key = "IZAACK_GAUSS",
        name = "Izaack",
        last_name = "Gauss",
        id_number = "456985512369",
        id_expiration = "09/1965",
        apartment = "F02-02",
        phone = "7332",
        occupation = "Reportero",
        features = "Cejas grandes, sonrisa amplia, barbilla prominente.",
        photo_asset = "Frame_photo.png"
    },
    {
        key = "STEVEN_RUDBOYS",
        name = "Steven",
        last_name = "Rudboys",
        id_number = "565845455965",
        id_expiration = "02/1966",
        apartment = "F03-03",
        phone = "4242",
        occupation = "Piloto",
        features = "Nariz grande, usa lentes, cabello corto, cabeza pequena.",
        photo_asset = "Frame_photo.png"
    }
}

-- Complete Building Directory Organized by Floor
neighbors.floors = {
    [1] = {
        name = "PISO 1",
        apartments = {
            { apt = "F01-01", phone = "1152", tenants = "Roman Stilnsky / Lois Stilnsky", photo = "Char_Roman_Small_Photo.png", note = "Contador / Ama de casa" },
            { apt = "F01-02", phone = "2668", tenants = "Robertsky / Albertsky Peachman", photo = "Frame_photo.png", note = "Zapateros hermanos" },
            { apt = "F01-03", phone = "5513", tenants = "Angus Ciprianni", photo = "Frame_photo.png", note = "Empresario prestigioso" },
            { apt = "F01-04", phone = "6996", tenants = "Elenois / Selenne Sverchzt", photo = "Frame_photo.png", note = "Gemelas modelos" }
        }
    },
    [2] = {
        name = "PISO 2",
        apartments = {
            { apt = "F02-01", phone = "5123", tenants = "Arnold / Gloria Schmicht", photo = "Frame_photo.png", note = "Escritor / Banquera" },
            { apt = "F02-02", phone = "7332", tenants = "Izaack Gauss", photo = "Frame_photo.png", note = "Periodista investigador" },
            { apt = "F02-03", phone = "6431", tenants = "Margarette Bubbles", photo = "Frame_photo.png", note = "Costurera vecina" },
            { apt = "F02-04", phone = "1346", tenants = "Anastacha / Nacha Mikaelys", photo = "Frame_photo.png", note = "Estudiante / Cocinera" }
        }
    },
    [3] = {
        name = "PISO 3",
        apartments = {
            { apt = "F03-01", phone = "1425", tenants = "Dr. W. Afton / Mia Stone", photo = "Frame_photo.png", note = "Fisico / Profesora" },
            { apt = "F03-02", phone = "4122", tenants = "Francis Mosses", photo = "Char_Francis_Small_Photo.png", note = "Lechero del barrio" },
            { apt = "F03-03", phone = "4242", tenants = "Steven / Mclooy Rudboys", photo = "Frame_photo.png", note = "Piloto / Retirado" },
            { apt = "F03-04", phone = "4258", tenants = "Alf / Rafttellyn Cappuccin", photo = "Frame_photo.png", note = "Abogado / Ama de casa" }
        }
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
function neighbors.spawn_visitor(force_doppel, is_nightmare)
    local template_idx = math.random(1, 2) -- Focus on Roman and Francis who have full sprite assets
    local base = neighbors.database[template_idx]

    local mouth_anim_frames = {}
    if base.key == "ROMAN_STILNSKY" then
        mouth_anim_frames = {
            "Char_Roman_Mouth_01.png",
            "Char_Roman_Mouth_02.png",
            "Char_Roman_Mouth_03.png",
            "Char_Roman_Mouth_04.png",
            "Char_Roman_Mouth_05.png"
        }
    else
        mouth_anim_frames = {
            "Char_Francis_Mouth_Doppel_01.png",
            "Char_Francis_Mouth_Doppel_02.png",
            "Char_Francis_Mouth_Doppel_03.png"
        }
    end

    local is_fake = force_doppel
    if is_nightmare and is_fake == nil then
        is_fake = (math.random() > 0.2) -- 80% doppelgängers in Nightmare mode
    elseif is_fake == nil then
        is_fake = (math.random() > 0.5)
    end

    local visitor = {
        template = base,
        name = base.name,
        last_name = base.last_name,
        displayed_id = base.id_number,
        displayed_exp = base.id_expiration,
        displayed_apt = base.apartment,
        displayed_phone = base.phone,
        occupation = base.occupation,
        is_doppel = is_fake or false,
        flaws = {},
        physical_extra_eye = false,
        physical_uzumaki = false,
        physical_blood = false,
        physical_extra_arms = false,
        flaws_has_ears = false,
        flaws_has_eyebrows = false,
        flaws_has_eyes = false,
        flaws_has_nose = false,
        photo_tampered = false,
        photo_asset = base.photo_asset,
        mouth_frames = mouth_anim_frames,
        dialogues = {}
    }

    if visitor.is_doppel then
        -- Doppelgänger creepy eerie dialogues
        if is_nightmare then
            visitor.dialogues = {
                greeting     = "LA NOCHE SE CONSUME. NUESTRA FORMA RECLAMA ESTE LUGAR.",
                reason       = "DESPOJAR LA CARNE Y ASIMILAR A CADA RESIDENTE DEL EDIFICIO.",
                id_inquiry   = "TUS PAPELES HUMANOS SE DESINTEGRARAN EN EL ABISMO.",
                list_inquiry = "NO EXISTE REGISTRO QUE PUEDA DETENER LA ASIMILACION."
            }
        elseif base.key == "ROMAN_STILNSKY" then
            visitor.dialogues = {
                greeting     = "Saludos, guardia humano. Deseo entrar a mi... hogar.",
                reason       = "Deseo... estar adentro. Con los demas... inquilinos.",
                id_inquiry   = "El tiempo y los numeros son conceptos irrelevantes para nosotros.",
                list_inquiry = "Las listas se queman. Mi presencia es inevitable."
            }
        else
            visitor.dialogues = {
                greeting     = "Abre la puerta... tengo mucha... sed de sangre fresca.",
                reason       = "Vine a alimentarme de los residentes del tercer piso.",
                id_inquiry   = "Mi carnet es tan real como esta carne que llevo puesta.",
                list_inquiry = "No necesito que me anoten en ningun papel para pasar."
            }
        end

        local anomaly_count = is_nightmare and math.random(2, 3) or 1
        for a = 1, anomaly_count do
            local roll = math.random(1, 6)

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
                    local p_roll = math.random(1, 3)
                    if p_roll == 1 then
                        visitor.physical_extra_eye = true
                    elseif p_roll == 2 then
                        visitor.physical_uzumaki = true
                    else
                        visitor.physical_extra_arms = true
                    end
                else
                    local f_roll = math.random(1, 4)
                    if f_roll == 1 or is_nightmare then
                        visitor.physical_blood = true
                    end
                    if f_roll == 2 then
                        visitor.flaws_has_ears = true
                    elseif f_roll == 3 then
                        visitor.flaws_has_eyes = true
                    elseif f_roll == 4 then
                        visitor.flaws_has_nose = true
                    end
                end
                table.insert(visitor.flaws, neighbors.FLAW_TYPES.PHYSICAL_ANOMALY)
            elseif roll == 5 then
                -- Name not on today's list
                visitor.last_name = base.last_name .. "-Doppel"
                table.insert(visitor.flaws, neighbors.FLAW_TYPES.NOT_ON_TODAY_LIST)
            elseif roll == 6 then
                -- Tampered Photo
                visitor.photo_tampered = true
                if base.key == "FRANCIS_MOSSES" then
                    visitor.photo_asset = "Char_Francis_Small_Photo_Doppel_01.png"
                else
                    visitor.photo_asset = "Char_Roman_Doppel_01_Small_Photo.png"
                end
                table.insert(visitor.flaws, neighbors.FLAW_TYPES.WRONG_ID_NUMBER)
            end
        end
    else
        -- Legitimate Human Citizen dialogues
        if base.key == "ROMAN_STILNSKY" then
            visitor.dialogues = {
                greeting     = "Buenas tardes. Regreso de mi trabajo de contador en el banco central.",
                reason       = "Vengo a descansar y cenar con mi esposa Lois en el depto F01-01.",
                id_inquiry   = "Mi carnet esta en regla. Cualquier falla de fecha es error de imprenta del DDD.",
                list_inquiry = "Vivo aqui desde hace cinco anos, siempre he figurado en los registros."
            }
        else
            visitor.dialogues = {
                greeting     = "Buenas tardes. Traigo el reparto de leche para los vecinos del tercer piso.",
                reason       = "Hago las entregas del turno vespertino y luego voy a mi departamento F03-02.",
                id_inquiry   = "Mi credencial fue sellada y aprobada por la jefatura lechera del D.D.D.",
                list_inquiry = "Hago esta misma ruta a diario, compruébelo con el resto del edificio."
            }
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
