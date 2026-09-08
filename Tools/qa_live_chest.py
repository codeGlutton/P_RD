import time
import unreal
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
start = time.monotonic()
phase = 0
samples = 0
shown = 0
review = None
cut_start = 0
handle = None
def tick(dt):
    global phase, shown, samples, review, cut_start
    now = time.monotonic()
    try:
        if phase == 0 and now-start > 3:
            unreal.SystemLibrary.execute_console_command(editor.get_editor_world(), 'RD.Editor.StartVisualReviewPIE')
            phase = 1
        world = editor.get_game_world() if phase else None
        if world and phase == 1 and now-start > 6:
            unreal.SystemLibrary.execute_console_command(world, 'RD.VisualReview')
            review = unreal.WidgetLibrary.get_all_widgets_of_class(world, unreal.VisualReviewWidget)[0]
            review.replay_chest()
            shown = time.monotonic()
            phase = 2
        if world and phase == 2 and samples < 4 and now-shown > [.85,1.1,1.45,1.75][samples]:
            unreal.SystemLibrary.execute_console_command(world, 'Shot SHOWUI filename=D:/UnrealProjects/P_RD_develop_20260907/Saved/UI/ChestLiveVFX_' + str(samples) + '.png -nosuffix')
            actors = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.ChestRewardVFX)
            unreal.log('CHEST_VFX_ACTORS ' + str(len(actors)))
            if actors:
                a = actors[0]
                rt = a.get_editor_property('target')
            samples += 1
            if samples == 4: phase = 3
        if phase == 3 and now-shown > 4:
            actors = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.ChestRewardVFX)
            unreal.log('CHEST_VFX_AFTER_END ' + str(len(actors)))
            assert len(actors) == 0
            review.play_ally()
            cut_start = time.monotonic()
            phase = 4
        if phase == 4 and now-cut_start > 1.8:
            cuts = unreal.WidgetLibrary.get_all_widgets_of_class(world, unreal.SkillCutInWidget)
            assert len(cuts) == 1 and not cuts[0].is_cut_in_playing() and cuts[0].get_render_opacity() == 0
            unreal.log('ALLY_CUTIN_FINISHED_TRANSPARENT')
            review.play_monster()
            cut_start = time.monotonic()
            phase = 5
        if phase == 5 and now-cut_start > 1.8:
            cuts = unreal.WidgetLibrary.get_all_widgets_of_class(world, unreal.SkillCutInWidget)
            assert len(cuts) == 1 and not cuts[0].is_cut_in_playing() and cuts[0].get_render_opacity() == 0
            unreal.log('MONSTER_CUTIN_FINISHED_TRANSPARENT')
            unreal.EditorPythonScripting.set_keep_python_script_alive(False)
            unreal.unregister_slate_post_tick_callback(handle)
    except Exception as e:
        unreal.log_error(str(e))
        unreal.EditorPythonScripting.set_keep_python_script_alive(False)
        unreal.unregister_slate_post_tick_callback(handle)
handle = unreal.register_slate_post_tick_callback(tick)

