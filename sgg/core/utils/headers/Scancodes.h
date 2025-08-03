#pragma once

#include <string_view>
#include <array>
#include <unordered_map>

namespace graphics {

    /**
     * @brief Keyboard scancode enumeration
     *
     * Scan codes of keyboard keys. The values are directly copied from the SDL scancode.h header file.
     * These represent physical key positions independent of keyboard layout.
     */
    enum class Scancode : int {
        Unknown = 0,

        // Letters
        A = 4, B = 5, C = 6, D = 7, E = 8, F = 9, G = 10, H = 11, I = 12, J = 13,
        K = 14, L = 15, M = 16, N = 17, O = 18, P = 19, Q = 20, R = 21, S = 22, T = 23,
        U = 24, V = 25, W = 26, X = 27, Y = 28, Z = 29,

        // Numbers
        Num1 = 30, Num2 = 31, Num3 = 32, Num4 = 33, Num5 = 34,
        Num6 = 35, Num7 = 36, Num8 = 37, Num9 = 38, Num0 = 39,

        // Common keys
        Return = 40,
        Escape = 41,
        Backspace = 42,
        Tab = 43,
        Space = 44,

        // Punctuation
        Minus = 45,
        Equals = 46,
        LeftBracket = 47,
        RightBracket = 48,
        Backslash = 49,
        NonUSHash = 50,
        Semicolon = 51,
        Apostrophe = 52,
        Grave = 53,
        Comma = 54,
        Period = 55,
        Slash = 56,

        CapsLock = 57,

        // Function keys
        F1 = 58, F2 = 59, F3 = 60, F4 = 61, F5 = 62, F6 = 63,
        F7 = 64, F8 = 65, F9 = 66, F10 = 67, F11 = 68, F12 = 69,

        // Navigation and special keys
        PrintScreen = 70,
        ScrollLock = 71,
        Pause = 72,
        Insert = 73,
        Home = 74,
        PageUp = 75,
        Delete = 76,
        End = 77,
        PageDown = 78,
        Right = 79,
        Left = 80,
        Down = 81,
        Up = 82,

        NumLockClear = 83,

        // Keypad
        KP_Divide = 84,
        KP_Multiply = 85,
        KP_Minus = 86,
        KP_Plus = 87,
        KP_Enter = 88,
        KP_1 = 89, KP_2 = 90, KP_3 = 91, KP_4 = 92, KP_5 = 93,
        KP_6 = 94, KP_7 = 95, KP_8 = 96, KP_9 = 97, KP_0 = 98,
        KP_Period = 99,

        NonUSBackslash = 100,

        // Extended keys
        Application = 101,
        Power = 102,
        KP_Equals = 103,

        // Extended function keys
        F13 = 104, F14 = 105, F15 = 106, F16 = 107, F17 = 108, F18 = 109,
        F19 = 110, F20 = 111, F21 = 112, F22 = 113, F23 = 114, F24 = 115,

        // System keys
        Execute = 116,
        Help = 117,
        Menu = 118,
        Select = 119,
        Stop = 120,
        Again = 121,
        Undo = 122,
        Cut = 123,
        Copy = 124,
        Paste = 125,
        Find = 126,

        // Audio controls
        Mute = 127,
        VolumeUp = 128,
        VolumeDown = 129,

        // Additional keypad keys
        KP_Comma = 133,
        KP_EqualsAS400 = 134,

        // International keys
        International1 = 135, International2 = 136, International3 = 137,
        International4 = 138, International5 = 139, International6 = 140,
        International7 = 141, International8 = 142, International9 = 143,

        // Language keys
        Lang1 = 144, Lang2 = 145, Lang3 = 146, Lang4 = 147, Lang5 = 148,
        Lang6 = 149, Lang7 = 150, Lang8 = 151, Lang9 = 152,

        // System keys continued
        AltErase = 153,
        SysReq = 154,
        Cancel = 155,
        Clear = 156,
        Prior = 157,
        Return2 = 158,
        Separator = 159,
        Out = 160,
        Oper = 161,
        ClearAgain = 162,
        CrSel = 163,
        ExSel = 164,

        // Extended keypad
        KP_00 = 176,
        KP_000 = 177,
        ThousandsSeparator = 178,
        DecimalSeparator = 179,
        CurrencyUnit = 180,
        CurrencySubunit = 181,
        KP_LeftParen = 182,
        KP_RightParen = 183,
        KP_LeftBrace = 184,
        KP_RightBrace = 185,
        KP_Tab = 186,
        KP_Backspace = 187,
        KP_A = 188, KP_B = 189, KP_C = 190, KP_D = 191, KP_E = 192, KP_F = 193,
        KP_XOR = 194,
        KP_Power = 195,
        KP_Percent = 196,
        KP_Less = 197,
        KP_Greater = 198,
        KP_Ampersand = 199,
        KP_DblAmpersand = 200,
        KP_VerticalBar = 201,
        KP_DblVerticalBar = 202,
        KP_Colon = 203,
        KP_Hash = 204,
        KP_Space = 205,
        KP_At = 206,
        KP_Exclam = 207,
        KP_MemStore = 208,
        KP_MemRecall = 209,
        KP_MemClear = 210,
        KP_MemAdd = 211,
        KP_MemSubtract = 212,
        KP_MemMultiply = 213,
        KP_MemDivide = 214,
        KP_PlusMinus = 215,
        KP_Clear = 216,
        KP_ClearEntry = 217,
        KP_Binary = 218,
        KP_Octal = 219,
        KP_Decimal = 220,
        KP_Hexadecimal = 221,

        // Modifier keys
        LeftCtrl = 224,
        LeftShift = 225,
        LeftAlt = 226,
        LeftGUI = 227,
        RightCtrl = 228,
        RightShift = 229,
        RightAlt = 230,
        RightGUI = 231,

        Mode = 257,

        // Media keys
        AudioNext = 258,
        AudioPrev = 259,
        AudioStop = 260,
        AudioPlay = 261,
        AudioMute = 262,
        MediaSelect = 263,

        // Application keys
        WWW = 264,
        Mail = 265,
        Calculator = 266,
        Computer = 267,
        AC_Search = 268,
        AC_Home = 269,
        AC_Back = 270,
        AC_Forward = 271,
        AC_Stop = 272,
        AC_Refresh = 273,
        AC_Bookmarks = 274,

        // Display controls
        BrightnessDown = 275,
        BrightnessUp = 276,
        DisplaySwitch = 277,

        // Keyboard illumination
        KbdIllumToggle = 278,
        KbdIllumDown = 279,
        KbdIllumUp = 280,

        // System controls
        Eject = 281,
        Sleep = 282,

        // Custom application keys
        App1 = 283,
        App2 = 284,

        // Additional audio controls
        AudioRewind = 285,
        AudioFastForward = 286,

        // Sentinel value
        NumScancodes = 512
    };

    // Legacy typedef for backward compatibility
    using scancode_t = Scancode;

    // Utility functions for scancode operations
    namespace ScancodeMaps {
        /**
         * @brief Convert scancode to human-readable string
         * @param scancode The scancode to convert
         * @return String representation of the scancode
         */
        [[nodiscard]] constexpr std::string_view toString(Scancode scancode) noexcept;

        /**
         * @brief Check if scancode represents a letter key
         * @param scancode The scancode to check
         * @return true if the scancode is a letter (A-Z)
         */
        [[nodiscard]] constexpr bool isLetter(Scancode scancode) noexcept {
            return scancode >= Scancode::A && scancode <= Scancode::Z;
        }

        /**
         * @brief Check if scancode represents a number key
         * @param scancode The scancode to check
         * @return true if the scancode is a number (0-9)
         */
        [[nodiscard]] constexpr bool isNumber(Scancode scancode) noexcept {
            return scancode >= Scancode::Num0 && scancode <= Scancode::Num9;
        }

        /**
         * @brief Check if scancode represents a function key
         * @param scancode The scancode to check
         * @return true if the scancode is a function key (F1-F24)
         */
        [[nodiscard]] constexpr bool isFunctionKey(Scancode scancode) noexcept {
            return (scancode >= Scancode::F1 && scancode <= Scancode::F12) ||
                   (scancode >= Scancode::F13 && scancode <= Scancode::F24);
        }

        /**
         * @brief Check if scancode represents a modifier key
         * @param scancode The scancode to check
         * @return true if the scancode is a modifier key (Ctrl, Shift, Alt, GUI)
         */
        [[nodiscard]] constexpr bool isModifier(Scancode scancode) noexcept {
            return scancode >= Scancode::LeftCtrl && scancode <= Scancode::RightGUI;
        }

        /**
         * @brief Check if scancode represents a keypad key
         * @param scancode The scancode to check
         * @return true if the scancode is from the numeric keypad
         */
        [[nodiscard]] constexpr bool isKeypad(Scancode scancode) noexcept {
            return (scancode >= Scancode::KP_Divide && scancode <= Scancode::KP_Period) ||
                   (scancode >= Scancode::KP_Equals && scancode <= Scancode::KP_Hexadecimal);
        }

        /**
         * @brief Check if scancode is valid
         * @param scancode The scancode to validate
         * @return true if the scancode is within valid range
         */
        [[nodiscard]] constexpr bool isValid(Scancode scancode) noexcept {
            const int code = static_cast<int>(scancode);
            return code >= 0 && code < static_cast<int>(Scancode::NumScancodes);
        }
    }

    // String conversion implementation
    constexpr std::string_view ScancodeMaps::toString(Scancode scancode) noexcept {
        switch (scancode) {
            case Scancode::Unknown: return "Unknown";
            case Scancode::A: return "A"; case Scancode::B: return "B"; case Scancode::C: return "C";
            case Scancode::D: return "D"; case Scancode::E: return "E"; case Scancode::F: return "F";
            case Scancode::G: return "G"; case Scancode::H: return "H"; case Scancode::I: return "I";
            case Scancode::J: return "J"; case Scancode::K: return "K"; case Scancode::L: return "L";
            case Scancode::M: return "M"; case Scancode::N: return "N"; case Scancode::O: return "O";
            case Scancode::P: return "P"; case Scancode::Q: return "Q"; case Scancode::R: return "R";
            case Scancode::S: return "S"; case Scancode::T: return "T"; case Scancode::U: return "U";
            case Scancode::V: return "V"; case Scancode::W: return "W"; case Scancode::X: return "X";
            case Scancode::Y: return "Y"; case Scancode::Z: return "Z";

            case Scancode::Num1: return "1"; case Scancode::Num2: return "2"; case Scancode::Num3: return "3";
            case Scancode::Num4: return "4"; case Scancode::Num5: return "5"; case Scancode::Num6: return "6";
            case Scancode::Num7: return "7"; case Scancode::Num8: return "8"; case Scancode::Num9: return "9";
            case Scancode::Num0: return "0";

            case Scancode::Return: return "Return";
            case Scancode::Escape: return "Escape";
            case Scancode::Backspace: return "Backspace";
            case Scancode::Tab: return "Tab";
            case Scancode::Space: return "Space";

            case Scancode::F1: return "F1"; case Scancode::F2: return "F2"; case Scancode::F3: return "F3";
            case Scancode::F4: return "F4"; case Scancode::F5: return "F5"; case Scancode::F6: return "F6";
            case Scancode::F7: return "F7"; case Scancode::F8: return "F8"; case Scancode::F9: return "F9";
            case Scancode::F10: return "F10"; case Scancode::F11: return "F11"; case Scancode::F12: return "F12";

            case Scancode::Left: return "Left"; case Scancode::Right: return "Right";
            case Scancode::Up: return "Up"; case Scancode::Down: return "Down";

            case Scancode::LeftCtrl: return "Left Ctrl"; case Scancode::RightCtrl: return "Right Ctrl";
            case Scancode::LeftShift: return "Left Shift"; case Scancode::RightShift: return "Right Shift";
            case Scancode::LeftAlt: return "Left Alt"; case Scancode::RightAlt: return "Right Alt";

            default: return "Unknown";
        }
    }

} // namespace graphics