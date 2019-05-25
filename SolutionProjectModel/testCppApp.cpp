#include "Project.h"
#include <filesystem>
#include <algorithm>                //transform

using namespace std;
using namespace filesystem;

void __declspec(dllexport) main(void)
{
    //Project proj(L"sharedItems");
    //proj.New(projecttype_CppSharedItemsProject);

    //proj.SetSaveDirectory(L".");
    //proj.Save();

    Project proj(L"gn");

    proj.SetSaveDirectory(LR"(C:\PrototypingQuick\CrashPad\gn)");
    //proj.SetVsVersion(2019);
    proj.AddPlatform(L"x64");

    string files[] =
    {
        "base/callback_internal.cc",
        "base/command_line.cc",
        "base/environment.cc",
        "base/files/file.cc",
        "base/files/file_enumerator.cc",
        "base/files/file_path.cc",
        "base/files/file_path_constants.cc",
        "base/files/file_util.cc",
        "base/files/scoped_file.cc",
        "base/files/scoped_temp_dir.cc",
        "base/json/json_parser.cc",
        "base/json/json_reader.cc",
        "base/json/json_writer.cc",
        "base/json/string_escape.cc",
        "base/logging.cc",
        "base/md5.cc",
        "base/memory/ref_counted.cc",
        "base/memory/weak_ptr.cc",
        "base/sha1.cc",
        "base/strings/string_number_conversions.cc",
        "base/strings/string_piece.cc",
        "base/strings/string_split.cc",
        "base/strings/string_util.cc",
        "base/strings/string_util_constants.cc",
        "base/strings/stringprintf.cc",
        "base/strings/utf_string_conversion_utils.cc",
        "base/strings/utf_string_conversions.cc",
        "base/third_party/icu/icu_utf.cc",
        "base/timer/elapsed_timer.cc",
        "base/value_iterators.cc",
        "base/values.cc",

        "tools/gn/action_target_generator.cc",
        "tools/gn/action_values.cc",
        "tools/gn/analyzer.cc",
        "tools/gn/args.cc",
        "tools/gn/binary_target_generator.cc",
        "tools/gn/builder.cc",
        "tools/gn/builder_record.cc",
        "tools/gn/build_settings.cc",
        "tools/gn/bundle_data.cc",
        "tools/gn/bundle_data_target_generator.cc",
        "tools/gn/bundle_file_rule.cc",
        "tools/gn/c_include_iterator.cc",
        "tools/gn/c_substitution_type.cc",
        "tools/gn/c_tool.cc",
        "tools/gn/command_analyze.cc",
        "tools/gn/command_args.cc",
        "tools/gn/command_check.cc",
        "tools/gn/command_clean.cc",
        "tools/gn/command_desc.cc",
        "tools/gn/command_format.cc",
        "tools/gn/command_gen.cc",
        "tools/gn/command_help.cc",
        "tools/gn/command_meta.cc",
        "tools/gn/command_ls.cc",
        "tools/gn/command_path.cc",
        "tools/gn/command_refs.cc",
        "tools/gn/commands.cc",
        "tools/gn/compile_commands_writer.cc",
        "tools/gn/config.cc",
        "tools/gn/config_values.cc",
        "tools/gn/config_values_extractors.cc",
        "tools/gn/config_values_generator.cc",
        "tools/gn/copy_target_generator.cc",
        "tools/gn/create_bundle_target_generator.cc",
        "tools/gn/deps_iterator.cc",
        "tools/gn/desc_builder.cc",
        "tools/gn/eclipse_writer.cc",
        "tools/gn/err.cc",
        "tools/gn/escape.cc",
        "tools/gn/exec_process.cc",
        "tools/gn/filesystem_utils.cc",
        "tools/gn/function_exec_script.cc",
        "tools/gn/function_foreach.cc",
        "tools/gn/function_forward_variables_from.cc",
        "tools/gn/function_get_label_info.cc",
        "tools/gn/function_get_path_info.cc",
        "tools/gn/function_get_target_outputs.cc",
        "tools/gn/function_process_file_template.cc",
        "tools/gn/function_read_file.cc",
        "tools/gn/function_rebase_path.cc",
        "tools/gn/functions.cc",
        "tools/gn/function_set_defaults.cc",
        "tools/gn/function_set_default_toolchain.cc",
        "tools/gn/functions_target.cc",
        "tools/gn/function_template.cc",
        "tools/gn/function_toolchain.cc",
        "tools/gn/function_write_file.cc",
        "tools/gn/general_tool.cc",
        "tools/gn/generated_file_target_generator.cc",
        "tools/gn/group_target_generator.cc",
        "tools/gn/header_checker.cc",
        "tools/gn/import_manager.cc",
        "tools/gn/inherited_libraries.cc",
        "tools/gn/input_conversion.cc",
        "tools/gn/input_file.cc",
        "tools/gn/input_file_manager.cc",
        "tools/gn/item.cc",
        "tools/gn/json_project_writer.cc",
        "tools/gn/label.cc",
        "tools/gn/label_pattern.cc",
        "tools/gn/lib_file.cc",
        "tools/gn/loader.cc",
        "tools/gn/location.cc",
        "tools/gn/metadata.cc",
        "tools/gn/metadata_walk.cc",
        "tools/gn/ninja_action_target_writer.cc",
        "tools/gn/ninja_binary_target_writer.cc",
        "tools/gn/ninja_build_writer.cc",
        "tools/gn/ninja_bundle_data_target_writer.cc",
        "tools/gn/ninja_c_binary_target_writer.cc",
        "tools/gn/ninja_copy_target_writer.cc",
        "tools/gn/ninja_create_bundle_target_writer.cc",
        "tools/gn/ninja_generated_file_target_writer.cc",
        "tools/gn/ninja_group_target_writer.cc",
        "tools/gn/ninja_target_command_util.cc",
        "tools/gn/ninja_target_writer.cc",
        "tools/gn/ninja_toolchain_writer.cc",
        "tools/gn/ninja_utils.cc",
        "tools/gn/ninja_writer.cc",
        "tools/gn/operators.cc",
        "tools/gn/output_conversion.cc",
        "tools/gn/output_file.cc",
        "tools/gn/parse_node_value_adapter.cc",
        "tools/gn/parser.cc",
        "tools/gn/parse_tree.cc",
        "tools/gn/path_output.cc",
        "tools/gn/pattern.cc",
        "tools/gn/pool.cc",
        "tools/gn/qt_creator_writer.cc",
        "tools/gn/runtime_deps.cc",
        "tools/gn/scheduler.cc",
        "tools/gn/scope.cc",
        "tools/gn/scope_per_file_provider.cc",
        "tools/gn/settings.cc",
        "tools/gn/setup.cc",
        "tools/gn/source_dir.cc",
        "tools/gn/source_file.cc",
        "tools/gn/standard_out.cc",
        "tools/gn/string_utils.cc",
        "tools/gn/substitution_list.cc",
        "tools/gn/substitution_pattern.cc",
        "tools/gn/substitution_type.cc",
        "tools/gn/substitution_writer.cc",
        "tools/gn/switches.cc",
        "tools/gn/target.cc",
        "tools/gn/target_generator.cc",
        "tools/gn/template.cc",
        "tools/gn/token.cc",
        "tools/gn/tokenizer.cc",
        "tools/gn/tool.cc",
        "tools/gn/toolchain.cc",
        "tools/gn/trace.cc",
        "tools/gn/value.cc",
        "tools/gn/value_extractors.cc",
        "tools/gn/variables.cc",
        "tools/gn/visibility.cc",
        "tools/gn/visual_studio_utils.cc",
        "tools/gn/visual_studio_writer.cc",
        "tools/gn/xcode_object.cc",
        "tools/gn/xcode_writer.cc",
        "tools/gn/xml_element_writer.cc",
        "util/exe_path.cc",
        "util/msg_loop.cc",
        "util/semaphore.cc",
        "util/sys_info.cc",
        "util/ticks.cc",
        "util/worker_pool.cc",


        "base/files/file_enumerator_win.cc",
        "base/files/file_util_win.cc",
        "base/files/file_win.cc",
        "base/win/registry.cc",
        "base/win/scoped_handle.cc",
        "base/win/scoped_process_information.cc",

        "tools/gn/gn_main.cc"
    };

    for (auto strPath : files)
        proj.AddFile(path(strPath).c_str());

    wstring defines = L"NDEBUG;NOMINMAX;UNICODE;WIN32_LEAN_AND_MEAN;WINVER=0x0A00;_CRT_SECURE_NO_DEPRECATE;_SCL_SECURE_NO_DEPRECATE;_UNICODE;_WIN32_WINNT=0x0A00";

    proj.VisitConfigurations( 
        [&](VCConfiguration& c)
        {
            c.General.IntDir = LR"(obj\$(ProjectName)_$(Configuration)_$(Platform)\)";
            c.General.OutDir = LR"(bin\$(Configuration)_$(Platform)\)";
            c.General.UseDebugLibraries = true;
            c.General.LinkIncremental = true;
            c.CCpp.Optimization.Optimization = optimization_Disabled;
            c.CCpp.General.AdditionalIncludeDirectories = L".;out";
            c.CCpp.General.MultiProcessorCompilation = true;
            c.CCpp.Preprocessor.PreprocessorDefinitions = defines + L";%(PreprocessorDefinitions)";
            c.Linker.System.SubSystem = subsystem_Console;
            c.Linker.Input.AdditionalDependencies = L"ws2_32.lib;Shlwapi.lib;%(AdditionalDependencies)";
            c.Linker.Debugging.GenerateDebugInformation = debuginfo_true;
        }
    );

    //auto f = proj.File(L"..\\SolutionProjectModel.dll", true);
    //f->General.ItemType = CustomBuild;
    //f->VisitTool(
    //    [](PlatformConfigurationProperties* props)
    //    {
    //        CustomBuildToolProperties& custtool= *((CustomBuildToolProperties*)props);
    //        CStringW cmd = "..\\cppexec.exe %(FullPath) >$(IntermediateOutputPath)%(Filename).def";
    //        cmd += "\n";
    //        cmd += "lib /nologo /def:$(IntermediateOutputPath)%(Filename).def /machine:$(Platform) /out:$(IntermediateOutputPath)%(Filename)_lib.lib";
    //        custtool.Message = "Generating static library for %(Identity)...";
    //        custtool.Command = cmd;
    //        custtool.Outputs = "$(IntermediateOutputPath)%(Filename)_lib.lib";
    //    }
    //, &CustomBuildToolProperties::GetType());

    proj.Save();

    //Project proj(L"emptyProject");
    //proj.SetSaveDirectory(L".");
    //proj.AddPlatform(L"x64");
    //proj.Save();

}

