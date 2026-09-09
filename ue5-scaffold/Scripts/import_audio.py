# Night Shift — Floor 37 | Sprint AF: import synthesized WAVs as SoundWaves.
#   UnrealEditor-Cmd <proj>.uproject -run=pythonscript -script=Scripts/import_audio.py -unattended -nop4 -nosplash
# Content/Imported/Audio/Synth/*.wav → /Game/Imported/Audio/<stem>; files starting with "amb_" loop.
import os, glob, unreal
src = os.path.join(unreal.Paths.project_content_dir(), "Imported", "Audio", "Synth")
dest = "/Game/Imported/Audio"
tools = unreal.AssetToolsHelpers.get_asset_tools()
for path in sorted(glob.glob(os.path.join(src, "*.wav"))):
    stem = os.path.splitext(os.path.basename(path))[0]
    task = unreal.AssetImportTask()
    task.filename = path; task.destination_path = dest; task.destination_name = stem
    task.automated = True; task.save = False; task.replace_existing = True
    tools.import_asset_tasks([task])
    snd = unreal.load_asset("%s/%s" % (dest, stem))
    if not snd:
        unreal.log_error("NS_AUDIO: failed %s" % path); continue
    if stem.startswith("amb_"):
        snd.set_editor_property("looping", True)
    unreal.EditorAssetLibrary.save_loaded_asset(snd)
    unreal.log("NS_AUDIO: %s/%s dur=%.2f loop=%s" % (dest, stem, snd.get_editor_property("duration"), stem.startswith("amb_")))
