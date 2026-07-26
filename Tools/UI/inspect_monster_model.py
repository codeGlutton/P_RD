"""One-off structured inspection of the Stage 1 eagle model Blueprint."""
import json
import os

import unreal


PATH = "/Game/BP/Pawn/Enemy/Stage1/BP_EagleUnit"
OUT = os.path.join(
    unreal.Paths.project_saved_dir(), "UI", "MonsterIconReferences",
    "eagle_model_inspection.json")

blueprint = unreal.EditorAssetLibrary.load_asset(PATH)
generated = unreal.EditorAssetLibrary.load_blueprint_class(PATH)
cdo = unreal.get_default_object(generated)

interesting = [
    name for name in dir(cdo)
    if any(token in name.lower() for token in
           ("mesh", "model", "component", "visual", "body", "actor"))
]

result = {
    "blueprint_class": blueprint.get_class().get_name(),
    "generated_class": generated.get_name(),
    "cdo_class": cdo.get_class().get_name(),
    "interesting_attributes": interesting,
}

if isinstance(cdo, unreal.Actor):
    components = cdo.get_components_by_class(unreal.ActorComponent)
    result["components"] = [
        {
            "name": component.get_name(),
            "class": component.get_class().get_name(),
            "mesh": str(component.get_editor_property("skeletal_mesh"))
                    if isinstance(component, unreal.SkeletalMeshComponent)
                    else "",
        }
        for component in components
    ]

os.makedirs(os.path.dirname(OUT), exist_ok=True)
with open(OUT, "w", encoding="utf-8") as handle:
    json.dump(result, handle, ensure_ascii=False, indent=2)
unreal.log("[MonsterRefs] wrote " + OUT)
