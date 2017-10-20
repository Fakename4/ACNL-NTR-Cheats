#include "cheats.hpp"
#include "Helpers/QuickMenu.hpp"
#include "Helpers/Hook.hpp"
#include <cstring>

extern "C" void APT_Hook(void);
volatile u32    g_homeBtnWasPressed = 0;
volatile u32    g_aptHookReturnAddress = 0;

namespace CTRPluginFramework
{
    static Hook     g_aptHook;
    static void     InstallAPTHook(void)
    {
        static const u8     aptHomeButtonPattern[] =
        {
            0x00, 0xF0, 0x20, 0xE3, // NOP
            0xA1, 0x1A, 0x00, 0xEB, // BL   #0x6A8C
            0x00, 0x00, 0x50, 0xE3, // CMP  R0, #0
            0x00, 0xF0, 0x20, 0xE3, // NOP
            0x03, 0x00, 0x00, 0x1A, // BNE  #0x14
            0x00, 0x00, 0xDD, 0xE5, // LDRB R0, [SP]
            0x01, 0x00, 0x50, 0xE3, // CMP  R0, #1
            0x02, 0x00, 0xA0, 0x13, // MOVNE R0, #2
            0x9E, 0x1A, 0x00, 0xEB, // BL   #0x6A80
            0x04, 0x10, 0x94, 0xE5, // LDR  R1, [R4, #4]
            0x00, 0x00, 0x51, 0xE3, // CMP  R1, #0
            0x03, 0x00, 0x00, 0x0A  // BEQ  #0x14
        };

        u8 *targetAddr = memsearch((u8 *)0x00100000, aptHomeButtonPattern, Process::GetTextSize(), sizeof(aptHomeButtonPattern));

        if (targetAddr)
        {
            u32 address = (u32)targetAddr + 4;

            g_aptHookReturnAddress = address + 0x170;
            g_aptHook.Initialize(address, (u32)APT_Hook);
            g_aptHook.Enable();
        }
    }

    // This function is called on the plugin starts, before main
    void    PatchProcess(void)
    {
        // Install APT Hook to block home button
        InstallAPTHook();
    }
    
    #define MAJOR_VERSION       4
    #define MINOR_VERSION       0
    #define REVISION_VERSION    0
    #define STRINGIFY(x)        #x
    #define TOSTRING(x)         STRINGIFY(x)
    #define STRING_VERSION      "[" TOSTRING(MAJOR_VERSION) "." TOSTRING(MINOR_VERSION) "." TOSTRING(REVISION_VERSION) " Beta 3" "]"
    
    extern Region               g_region;
    static const std::string    unsupportedVersion = "Your ACNL version isn't\nsupported!\nMake sure you have the\n1.5 update installed!";
    static const std::string    unsupportedGame = "Error\nGame not supported !\nVisit discord for support.";
    static const std::string    gameName = "Animal Crossing New Leaf";
    static const std::string    developer = "RyDog";
    static const std::string    credits =
        "Plugin Version: " STRING_VERSION  "\n"
        "Creator: " + developer + "\n"
        "\n"
        "Special thanks to:\n"
        "Nanquitas\n"
        "Slattz\n"
        "Mega Mew\n"
        "Scotline\n"
        "and others :)";

    static bool    CheckRegion(void)
    {
        u64         tid = Process::GetTitleID();
        u16         ver = Process::GetVersion();

        // Get current game's region
        if (tid == 0x0004000000086300)
        {
            if (ver != 6192) goto unsupported;
            g_region = USA;
        }
        else if (tid == 0x0004000000086400)
        {
            if (ver != 6176) goto unsupported;
            g_region = EUR;
        }
        else if (tid == 0x0004000000086200)
        {
            if (ver != 6272) goto unsupported;
            g_region = JAP;
        }
        else
        {
            (MessageBox(unsupportedGame))();
            return (true);
        }

        return (false);

    unsupported:
        (MessageBox(unsupportedVersion))();
        return (true);
    }

    MenuEntry *EntryWithHotkey(MenuEntry *entry, const Hotkey &hotkey)
    {
        if (entry != nullptr)
        {
            entry->Hotkeys += hotkey;
            entry->SetArg(new std::string(entry->Name()));
            entry->Name() += " " + hotkey.ToString();
            entry->Hotkeys.OnHotkeyChangeCallback([](MenuEntry *entry, int index)
            {
                std::string *name = reinterpret_cast<std::string *>(entry->GetArg());

                entry->Name() = *name + " " + entry->Hotkeys[0].ToString();
            });
        }

        return (entry);
    }

    MenuEntry *EntryWithHotkey(MenuEntry *entry, const std::vector<Hotkey> &hotkeys)
    {
        if (entry != nullptr)
        {
            for (const Hotkey &hotkey : hotkeys)
                entry->Hotkeys += hotkey;
        }

        return (entry);
    }
    
    void    InitQuickMenu(void);
    int     main(void)
    {
        PluginMenu  *m = new PluginMenu(gameName, MAJOR_VERSION, MINOR_VERSION, REVISION_VERSION, credits);
        PluginMenu  &menu = *m;

        if (CheckRegion())
            return (1); ///< Unsupported game/version

        // Initialize game's addresses based on region
        Game::Initialize();
        // Initialize player (will block plugin until the user loaded his savegame)
        Player::GetInstance();
        // Change QuickMenu's hotkey
        QuickMenu::GetInstance().ChangeHotkey(R + X);
        // Init QuickMenu
        InitQuickMenu();
        // T&C Message & Save Backup Message
        StartMsg();

        /*
        ** Garden
        ********************/

        menu += new MenuFolder("Save File Codes", std::vector<MenuEntry *>(
        {
            new MenuEntry("Set name to...", nullptr, SetNameTo),
            new MenuEntry("Save Dumper", nullptr, GardenDumper, "Select the keyboard icon to start dumping your save file."),
            new MenuEntry("Save Restore", nullptr, GardenRestore, "Select this icon to open file picker to restore from your previously dumped saves"),
            new MenuEntry("Change Town Fruit to...", nullptr, ChangeNativeFruit, "Special thanks to Mega Mew and Scotline"),
            new MenuEntry("Change Town Grass to...", nullptr, ChangeGrass, "Special thanks to Mega Mew and Scotline"),
            new MenuEntry("Real Time Building Placer", nullptr, BuildingPlacer, "Press on the keyboard option to bring up the building keyboard."),
        }));


        /*
        ** Movement
        ********************/

        menu += new MenuFolder("Movement Codes", std::vector<MenuEntry *>(
        {
            EntryWithHotkey(new MenuEntry("Coordinates Modifier", CoordinateModifier, "Press an hotkey to move to the corresponding direction."),
                {Hotkey(Key::A | Key::DPadUp, "Go up"), Hotkey(Key::A | Key::DPadDown, "Go down"), Hotkey(Key::A | Key::DPadLeft, "Go left") , Hotkey(Key::A | Key::DPadRight, "Go right")}),
            new MenuEntry("Touch Coordinates", TouchCoordinates, "Touch the map to teleport your character there."),
            EntryWithHotkey(new MenuEntry("Teleport", Teleporter, "Press the hotkey to save/restore your location. You can use a slot modifier hotkey together to change the slot that'll be used."),
                {Hotkey(Key::B | Key::DPadUp, "Save current location"), Hotkey(Key::B | Key::DPadDown, "Restore saved location"),
                 Hotkey(Key::L, "Use slot 2"), Hotkey(Key::R, "Use slot 3") }),
            new MenuEntry("Walk Over Things", WalkOverThings, "Press \uE052 and \uE079 to enable walking through stuff, \uE052 and \uE07A to disable walking through stuff."),
            new MenuEntry("Speed Hack", SpeedHack, SpeedHackEditor, "Change how fast you want to go with the keyboard icon\nCredits to Mega Mew for this cheat"),
            new MenuEntry("Moon Jump", MoonJump, "Press \uE052 and \uE079 to go higher and \uE07A to go lower."),
            new MenuEntry("Teleport to PWP...", nullptr, PWPTeleport, "Press on the keyboard to open up the menu to choose which PWP to teleport to")
        }));

        /*
        ** Main Street
        ********************/

        menu += new MenuFolder("Main Street Codes", std::vector<MenuEntry *>(
        {
            new MenuEntry("Nookling Upgrades", nullptr, NooklingKeyboard, "Press the keyboard icon to change which upgrade the Nooklings have"),
            new MenuEntry("Fill out Catalog", FillCatalog, "Fill out the catalog in Nookling's shop"),
            new MenuEntry("Fill out Main Street", FillMainStreet, "Unlocks all the Main Street buildings except Leif + Nooklings"),
            new MenuEntry("Catalog to Pockets", CatalogToPockets, "Press " FONT_L " and " FONT_A " while in the catalog to send the item directly to your pockets!")
        }));

        /*
        ** Time Travel
        ********************/

        menu += new MenuFolder("Time Travel Codes", std::vector<MenuEntry *>(
        {
            new MenuEntry("Time Travel", TimeTravel, TimeTravelSettings, "Press either " FONT_R " or " FONT_B " and " FONT_DR " to travel forward or " FONT_DL " to retwind time or " FONT_B " and " FONT_DD " to set ingame time back to your 3DS's clock."),
            new MenuEntry("Time Machine", TimeMachine, "Press " FONT_Y " and " FONT_DR " to start time traveling.")
        }));

        /*
        ** Inventory
        ********************/

        menu += new MenuFolder("Inventory", std::vector<MenuEntry *>(
        {
            new MenuEntry("Text to Item", Text2Item, "Press " FONT_X " and " FONT_DR " to open the keyboard to enter in the ID you want to recieve."),
            new MenuEntry("Duplicate", Duplication, "Press " FONT_R " to duplicate the item that is slot 01 to slot 02."),
            new MenuEntry("Show names of buried items", ShowBuriedItems, "This allows you to view what is buried underground in the little thought bubble above your head\nWarning: this is a heavy cheat, so it might cause slowdown."),
            new MenuEntry("Pick up buried items", PickBuriedItems, "Press " FONT_Y " to pick up any buried items.\nWarning: this is a heavy cheat, so it might cause slowdown."),
            new MenuEntry("Inventory box extender", ExtendedInventoryBox, "This allows you to create 10 additionals boxes to store your items.\nOnce activated, open the quick menu in-game to see the option Inventory Box."),
            new MenuEntry("Fossil Inspector", GenerateFossils, "Press " FONT_X " and " FONT_A " to process all fossils\nas if you talked to Blathers"),
            new MenuEntry("Max Bank", MaxMoneyBank),
            new MenuEntry("Infinite Wallet", InfiniteWallet),
            new MenuEntry("Infinite/Max Coupons", InfiniteCoupons),
            new MenuEntry("Infinite/Max Island Medals", InfiniteMedals),
            new MenuEntry("Wallet editor (0)", WalletEditor, WalletEditorSetter, "Touch the keyboard icon on the bottom screen to change the desired value"),
            new MenuEntry("Bank editor (0)", BankEditor, BankEditorSetter, "Touch the keyboard icon on the bottom screen to enter the desired amount of bells in your bank.")
        }));

        /*
        ** Environment
        ********************/

        MenuFolder *folder = new MenuFolder("Enviroment");
        {
            /* Subfolder of Enviroment
            ** These codes only execute when R+A is pressed, so I only want 1 enabled.
            ** I use the radio group parameter to prevent multiple from being enabled.
            *****************************************************************************/
            *folder += new MenuFolder("R + A Codes", std::vector<MenuEntry *>(
            {
                new MenuEntry(1, "Remove All Items", RemoveAllItems, "Press " FONT_R " and " FONT_A " to execute... Beware as there is no going back if you save."),
                new MenuEntry(1, "Remove All Weeds", RemoveAllWeeds, "Press " FONT_R " and " FONT_A " to execute."),
                new MenuEntry(1, "Water All Flowers", WaterAllFlowers, "Press " FONT_R " and " FONT_A "to execute."),
                new MenuEntry(1, "Fill Grass", FillGrass, "Press " FONT_R " and " FONT_A " to fill your town with grass.\nPlease note that bald spots will respawn on the next day."),
                new MenuEntry(1, "Destroy Grass", DestroyGrass, "Press " FONT_R " and " FONT_A " to destroy all the grass in your town to make a desert wasteland.")
            }));
            *folder += new MenuEntry("Real Time World Edit", WorldEdit, "Press " FONT_R " and " FONT_DL " to open the keyboard to store the item. " FONT_R " and " FONT_DU " to store the item that you're standing on. And " FONT_R " + " FONT_DD " to write the item to the place that you're standing on.");
            *folder += new MenuEntry("Search and Replace", nullptr, SearchReplace, "Press the keyboard icon to enter in what you want to search and replace");
        }
        menu += folder;

        /*
        ** Unlock
        ********************/

        menu += new MenuFolder("Unlock Codes", std::vector<MenuEntry *>(
        {
            new MenuEntry("100% Mayor permit", Permit, "Special thanks to Slattz"),
            new MenuEntry("Unlock all PWPs", PWPUnlock, "Special thanks to Mega Mew and Scotline"),
            new MenuEntry("Fill out encyclopedia", Encyclopedia, "Special thanks to Mega Mew and Scotline"),
            new MenuEntry("Fill out emoticons", Emoticons, "Special thanks to Mega Mew and Scotline"),
            new MenuEntry("Fill out K.K. Songs", Songs, "Special thanks to Mega Mew and Scotline")
        }));

        /*
        ** Misc.
        ********************/

        menu += new MenuFolder("Misc.", std::vector<MenuEntry *>(
        {
            new MenuEntry("Ghost Mode", GhostMode, "Press Y + D pad up to enable, Y + D pad down to disable"),
            new MenuEntry("Camera Mod", CameraMod, "R + Circle pad = rotation \n\n R + A stop player from moving or Y lock camera, X unlock camera(needed) \n\n B + D pad move camera, L / R up and down"),
            new MenuEntry("Custom Symbols Keyboard", CustomKB, "This turns all the symbols in the keyboard into Nintendo symbols.\nExample: \uE00F\uE004\uE000\uE00E\uE00E\uE04B"),
            new MenuEntry("Keyboard Extender", KeyboardExtender, "This extends the max characters that you can type into chat to 54 characters.\nSpecial thanks to Wii8461 for this cheat"),
            new MenuEntry("Fast Game Speed", FastGameSpeed, "This makes things in the game speed up. This might make your game crash.\nCredits to Scotline and Mega Mew for this cheat"),
            new MenuEntry("Item Form Changer", ItemFormChanger, ItemFormEditor, "This changes how your character holds tools"),
            new MenuEntry("Item Effect Changer", ItemEffectChanger, ItemEffectEditor, "This changes how your character uses items."),
            new MenuEntry("Special NPC Changer", AnimalChanger, AnimalChangerKeyboard, "This changes all the special NPC's like K.K. to what you choose."),
            new MenuEntry("Access Catalog & Storage Anywhere", StorageEverywhere, "Press L or R to access your storage, and press L+R to access the catalog while switching emoticon tabs\nSpecial thanks Mega Mew and Scotline for this cheat :)"),
            new MenuEntry("Faint", Faint, "Press R + A to make your character pass out like he got bit by a scorpion!\nCredits to Hikaru"),
            new MenuEntry("Test Weed", TestWeedCode)
            //new MenuEntry("Corrupter", Corrupter, CorrupterSettings, "WARNING!\nThis corrupts random values in memory to cause funny side effects.\nUse at own risk!"),
            //new MenuEntry("Animation changer", DoorChanger, DoorChangerSettings, "This changes what your animation is for going through doors\nCredits to 0ICED0!")
        }));

        /*
        ** Callbacks
        ********************/

        menu += []
        {
            Sleep(Milliseconds(1));
            QuickMenu::GetInstance()();
            if (g_homeBtnWasPressed)
            {
                g_homeBtnWasPressed = 0;
                OSD::Notify("The homebutton is disabled because of memory issue", Color::Red, Color::Blank);
            }
        };
        menu += PlayerUpdateCallback;
        menu += MiniGame;

        // Launch menu and mainloop
        menu.Run();

        // Exit plugin
        return (0);
    }
}
