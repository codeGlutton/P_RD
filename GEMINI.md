# Rogue the Dice - Project Context

## Project Overview
**Rogue the Dice** is a Strategy RPG (SRPG) developed with Unreal Engine 5.7. It combines traditional turn-based combat with rogue-like elements and a central dice-rolling mechanic. The project is designed with a heavy emphasis on data-driven systems, utilizing the Gameplay Ability System (GAS) and StateTree for core game logic and AI.

- **Primary Technologies**: Unreal Engine 5.7, Gameplay Ability System (GAS), StateTree, Enhanced Input, Substrate.
- **Target Platform**: Mobile (Android).
- **Source Control/Automation**: SVN (integrated via `SVNLinker.py`).

## Core Architecture

### 1. SRPG Framework
The combat and turn flow are managed by the `SRPGFrameworkSubsystem` (an `UInstanceSubsystem`).
- **Turn Management**: Uses a circular list to manage turn order.
- **Action Queue**: Turns are processed through `SRPGTurnContext`, which holds a queue of `SRPGAction` objects (e.g., Rolling Dice, Moving, Using Skills).
- **Extensibility**: The framework is designed to handle dynamic turn addition/removal and flexible action sequencing.

### 2. Gameplay Ability System (GAS)
GAS is the backbone of the game's combat mechanics.
- **Attributes**: Managed via `AttributeSet` with standard `ATTRIBUTE_ACCESSORS`.
- **Abilities**: Includes active skills (Attack, Spell) and complex passive triggers (OnStartTurn, OnRollingDice, OnHitting).
- **Tags**: Native Gameplay Tags are strictly organized into namespaces:
    - `InputTags`: User input mapping.
    - `AbilityTags`: Core and passive ability identifiers.
    - `EffectTags`: Costs, cooldowns, and status markers.
    - `EventTags`: Triggers for GAS events.
    - `CueTags`: Visual and audio feedback.

### 3. Data-Driven Design
The game uses `PrimaryDataAsset` to define various game entities, allowing for easy balancing and content creation.
- **Asset Types**:
    - `Stage`: Global stage definitions.
    - `Room`: Categorized by type (Monster, Elite, Boss, Shop, Treasure) and Stage Level.
    - `Unit`: Player and Enemy definitions.
    - `Equipment`: Weapon, Gloves, Boots, categorized by Rarity.
    - `Skill`: Attack and Spell skills, categorized by Rarity.
    - `Dice`: Dice variants categorized by Rarity.

## Development Conventions

### 1. Header Standards
- **`RDMinimal.h`**: The foundational header for the project. Includes common engine headers, math libraries, and project-specific utilities.
- **`GASMinimal.h`**: Included in GAS-related classes. Contains all necessary GAS headers and the `ATTRIBUTE_ACCESSORS` macro.
- **Doxygen Documentation**: Headers use Doxygen-style comments (`/** ... */`) for documenting classes, variables, and framework flows.

### 2. Code Style
- **Naming**: Follows standard Unreal Engine PascalCase conventions.
- **Localization**: While the code is in English, comments and documentation are primarily in Korean.
- **Macros**: Extensive use of macros for defining `PrimaryAssetType` and `GameplayTags` to ensure type safety and consistency.

### 3. Workflow
- **Editor Startup**: The `SVNLinker.py` script runs automatically to manage source control integration.
- **Validation**: Uses the Unreal `DataValidation` module to ensure assets are correctly configured.

## Building and Running
- **Project File**: `P_RD.uproject`
- **Modules**:
    - `P_RD`: Core runtime logic.
    - `P_RDEditor`: Custom editor tools and validators.
- **Build Commands**: Standard Unreal Build Tool (UBT) commands.
    - Compile: `UnrealBuildTool.exe P_RD Win64 Development "E:\Unreal\P_RD\P_RD\P_RD.uproject"`
    - Test: Use the Unreal Editor's internal testing tools or custom `Validator` modules in `P_RDEditor`.

## Directory Map
- `/Source/P_RD/GAS/`: GAS implementations (Abilities, Attributes, Tags).
- `/Source/P_RD/SRPGFramework/`: Core turn-based logic and action systems.
- `/Source/P_RD/DataAsset/`: C++ definitions for data-driven assets.
- `/Source/P_RDEditor/`: Editor-only extensions and asset validators.
- `/Content/BP/`: Blueprint implementations and asset instances.
- `/Doxygen/`: Documentation source and diagrams.
