#include "classinterface.h"

#include "widgets.h"
#include "hkx/hkclass.inl"
#include "hkx/hkutils.h"

#include <robin_hood.h>
#include <imgui.h>
#include <extern/font_awesome_5.h>

namespace Haviour
{
namespace Ui
{

constexpr auto EditAreaTableFlag = ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY;

#define UICLASS(hkclass) void show_##hkclass(pugi::xml_node obj, Hkx::HkxFile& file)

///////////////////////    NOT REGISTERED BUT COMMONLY USED

UICLASS(hkbStringEventPayload)
{
    stringEdit(obj.getByName("data"));
}

UICLASS(hkbExpressionCondition)
{
    stringEdit(obj.getByName("expression"));
}

UICLASS(hkbEvent)
{
    linkedPropPickerEdit(obj.getByName("id"), file, file.m_evt_manager);
    refEdit(obj.getByName("payload"), {"hkbStringEventPayload"}, file);
    if (auto payload = Hkx::HkxFileManager::getSingleton()->getCurrentFile().getObj(obj.getByName("payload").text().as_string()); payload)
    {
        ImGui::Indent(10);
        show_hkbStringEventPayload(payload, file);
        ImGui::Indent(-10);
    }
}
void hkbEventSelectable(pugi::xml_node obj, Hkx::HkxFile& file, const char* hint, const char* text)
{
    ImGui::PushID(text);

    ImGui::TableNextColumn();
    if (ImGui::Button(ICON_FA_PEN))
        ImGui::OpenPopup(text);
    ImGui::SameLine();
    if (ImGui::Selectable(text, ImGui::IsPopupOpen(text)))
        ImGui::OpenPopup(text);
    if (hint)
        addTooltip(hint);
    if (ImGui::BeginPopup(text, ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (ImGui::BeginTable("hkbEventSelectable", 2, ImGuiTableFlags_SizingStretchProp, {400.0f, -FLT_MIN}))
        {
            show_hkbEvent(obj, file);
            ImGui::EndTable();
        }
        ImGui::EndPopup();
    }
    ImGui::TableNextColumn();

    ImGui::PopID();
}

UICLASS(hkbStateMachineEventPropertyArray)
{
    auto events_node = obj.getByName("events");

    ImGui::AlignTextToFramePadding();
    ImGui::BulletText("events"), ImGui::SameLine();
    if (ImGui::Button(ICON_FA_PLUS_CIRCLE))
        appendXmlString(events_node, Hkx::g_def_hkbEvent);
    addTooltip("Add new event");
    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(events_node.attribute("numelements").as_string());

    constexpr auto table_flag = ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_Resizable |
        ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_NoBordersInBody;
    if (ImGui::BeginTable("events", 2, table_flag, ImVec2(-FLT_MIN, -FLT_MIN)))
    {
        pugi::xml_node mark_delete = {};
        bool           do_delete   = false;
        auto           numelements = events_node.attribute("numelements").as_ullong();
        size_t         i           = 0;
        for (auto event : events_node.children())
        {
            ImGui::PushID(i);
            hkbEventSelectable(event, file, {},
                               file.m_evt_manager.getEntry(event.getByName("id").text().as_ullong()).getName());
            if (ImGui::Button(ICON_FA_MINUS_CIRCLE))
            {
                do_delete   = true;
                mark_delete = event;
            }
            addTooltip("Remove");
            ImGui::PopID();

            ++i;
        }
        if (do_delete)
        {
            events_node.remove_child(mark_delete);
            events_node.attribute("numelements") = events_node.attribute("numelements").as_int() - 1;
        }

        ImGui::EndTable();
    }
}

UICLASS(hkbStateMachine_TimeInterval)
{
    linkedPropPickerEdit(obj.getByName("enterEventId"), file, file.m_evt_manager,
                         "The event ID that bounds the beginning of the interval (set to EVENT_ID_NULL to use m_enterTime instead).");
    linkedPropPickerEdit(obj.getByName("exitEventId"), file, file.m_evt_manager,
                         "The event ID that bounds the end of the interval (set to EVENT_ID_NULL to use m_exitTime instead).");
    floatEdit(obj.getByName("enterTime"), file,
              "The time at which the interval is entered (used if both m_enterEventId and m_exitEventId are set to EVENT_ID_NULL).");
    floatEdit(obj.getByName("exitTime"), file,
              "The time at which the interval is exited (used if both m_enterEventId and m_exitEventId are set to EVENT_ID_NULL).\n"
              "Setting this to 0.0f means infinity; the inverval will not end.");
}

UICLASS(hkbStateMachine_TransitionInfo)
{
    ImGui::TableNextColumn();
    ImGui::BulletText("triggerInterval");
    addTooltip("The interval in which the event must be received for the transition to occur.\n\n"
               "This is only used if (m_flags & FLAG_USE_TRIGGER_INTERVAL) is true.\n"
               "You should make sure that the interval is longer than your timestep (eg, 1/30 sec), or else the interval may be missed.");
    ImGui::Indent(10);
    if (ImGui::BeginTable("triggerInterval", 2, EditAreaTableFlag, {-FLT_MIN, 27 * 4}))
    {
        show_hkbStateMachine_TimeInterval(obj.getByName("triggerInterval").first_child(), file);
        ImGui::EndTable();
    }
    ImGui::Indent(-10);
    ImGui::TableNextRow();

    ImGui::TableNextColumn();
    ImGui::BulletText("initiateInterval");
    addTooltip("The interval in which the transition may begin.\n\n"
               "This is only used if (m_flags & FLAG_USE_BEGIN_INTERVAL) is true.\n"
               "If the transition is activated outside of this interval, the transition will be delayed until the interval begins.\n"
               "You should make sure that the interval is longer than your timestep (eg, 1/30 sec), or else the interval may be missed.");
    ImGui::Indent(10);
    if (ImGui::BeginTable("initiateInterval", 2, EditAreaTableFlag, {-FLT_MIN, 27 * 4}))
    {
        show_hkbStateMachine_TimeInterval(obj.getByName("initiateInterval").first_child(), file);
        ImGui::EndTable();
    }
    ImGui::Indent(-10);
    ImGui::TableNextRow();

    refEdit(obj.getByName("transition"), {"hkbBlendingTransitionEffect"}, file,
            "The transition to apply.\n\n"
            "This is public but should be handled with care since it is reference counted.\n"
            "It's best to change this only by calling methods of hkbStateMachine.");
    refEdit(obj.getByName("condition"), {"hkbExpressionCondition"}, file,
            "A condition that determines whether the transition should occur.\n\n"
            "The transition only occurs if this condition evaluates to true.\n"
            "If m_event == hkbEvent::EVENT_ID_NULL then the transition occurs as soon as the condition becomes true.\n"
            "Otherwise, the transition occurs when the event is received and the condition is also true.");
    auto condition = file.getObj(obj.getByName("condition").text().as_string());
    if (condition)
    {
        ImGui::Indent(10.0F);
        show_hkbExpressionCondition(condition, file);
        ImGui::Indent(-10.0F);
    }

    linkedPropPickerEdit(obj.getByName("eventId"), file, file.m_evt_manager);

    fullTableSeparator();

    auto state_machine = getParentStateMachine(obj, file);
    stateEdit(obj.getByName("toStateId"), file, state_machine,
              "The state to which the transition goes.");
    stateEdit(obj.getByName("fromNestedStateId"), file, state_machine,
              "The nested state (a state within the state machine that is inside the from-state) from which this transition must be done.");
    stateEdit(obj.getByName("toNestedStateId"), file, state_machine,
              "A nested state to which this transitions goes.");
    fullTableSeparator();
    intScalarEdit(obj.getByName("priority"), file, ImGuiDataType_S16,
                  "Each transition has a priority.");
    flagEdit(obj.getByName("flags"), Hkx::f_hkbStateMachine_TransitionInfo_TransitionFlags,
             "Flags indicating specialized behavior (see TransitionInfoTransitionFlagBits).");
}

UICLASS(hkbModifier)
{
    stringEdit(obj.getByName("name"));
    intScalarEdit(obj.getByName("userData"), file, ImGuiDataType_U32);
    refEdit(obj.getByName("variableBindingSet"), {"hkbVariableBindingSet"}, file);
    boolEdit(obj.getByName("enable"), file);
}

UICLASS(BSLookAtModifier_Bone)
{
    boolEdit(obj.getByName("enabled"), file);
    intScalarEdit(obj.getByName("index"), file, ImGuiDataType_S16);
    quadEdit(obj.getByName("fwdAxisLS"), file);
    floatEdit(obj.getByName("limitAngleDegrees"), file);
    floatEdit(obj.getByName("onGain"), file);
    floatEdit(obj.getByName("offGain"), file);
}
void BSLookAtModifierBoneArray(pugi::xml_node hkparam, Hkx::HkxFile& file)
{
    ImGui::AlignTextToFramePadding();
    ImGui::BulletText(hkparam.attribute("name").as_string()), ImGui::SameLine();
    if (ImGui::Button(ICON_FA_PLUS_CIRCLE))
        appendXmlString(hkparam, Hkx::g_def_BSLookAtModifier_Bone);
    addTooltip("Add new event");
    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(hkparam.attribute("numelements").as_string());

    constexpr auto table_flag = ImGuiTableFlags_SizingStretchProp |
        ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_NoBordersInBody;
    if (ImGui::BeginTable("events", 2, table_flag, ImVec2(-FLT_MIN, -FLT_MIN)))
    {
        pugi::xml_node mark_delete = {};
        bool           do_delete   = false;
        auto           numelements = hkparam.attribute("numelements").as_ullong();
        size_t         i           = 0;
        for (auto bone : hkparam.children())
        {
            auto disp_name = bone.getByName("index").text().as_string();

            ImGui::PushID(i);

            ImGui::TableNextColumn();
            if (ImGui::Button(ICON_FA_MINUS_CIRCLE))
            {
                do_delete   = true;
                mark_delete = bone;
            }
            addTooltip("Remove");
            ImGui::SameLine();
            if (ImGui::Selectable(disp_name, ImGui::IsPopupOpen(disp_name)))
                ImGui::OpenPopup(disp_name);
            if (ImGui::BeginPopup(disp_name, ImGuiWindowFlags_AlwaysAutoResize))
            {
                if (ImGui::BeginTable("hkbBehaviorGraph", 2, ImGuiTableFlags_SizingStretchProp, {400.0f, -FLT_MIN}))
                {
                    show_BSLookAtModifier_Bone(bone, file);
                    ImGui::EndTable();
                }
                ImGui::EndPopup();
            }
            ImGui::TableNextColumn();

            ImGui::PopID();

            ++i;
        }
        if (do_delete)
        {
            hkparam.remove_child(mark_delete);
            hkparam.attribute("numelements") = hkparam.attribute("numelements").as_int() - 1;
        }

        ImGui::EndTable();
    }
}

///////////////////////    REGISTERED INTERFACES

UICLASS(hkbBehaviorGraph)
{
    if (ImGui::BeginTable("hkbBehaviorGraph", 4, EditAreaTableFlag))
    {
        stringEdit(obj.getByName("name"));
        intScalarEdit(obj.getByName("userData"), file, ImGuiDataType_U32);
        refEdit(obj.getByName("variableBindingSet"), {"hkbVariableBindingSet"}, file);
        enumEdit(obj.getByName("variableMode"), Hkx::e_variableMode,
                 "How do deal with variables when the behavior is inactive.");
        refEdit(obj.getByName("rootGenerator"), {"hkbStateMachine"}, file,
                "The root node of the behavior graph.");
        refEdit(obj.getByName("data"), {"hkbBehaviorGraphData"}, file,
                "The constant data associated with the behavior.");
        ImGui::EndTable();
    }
}

UICLASS(hkbVariableBindingSet)
{
    static pugi::xml_node obj_cache;
    static pugi::xml_node edit_binding = {};
    if (obj_cache != obj)
        edit_binding = {};
    obj_cache = obj; // workaround to prevent crashing between files, asserting that may only be one menu at the same time

    if (ImGui::BeginTable("hkbBehaviorGraph1", 2, ImGuiTableFlags_SizingStretchProp))
    {
        intScalarEdit(obj.getByName("indexOfBindingToEnable"), file, ImGuiDataType_S32,
                      "If there is a binding to the member hkbModifier::m_enable then we store its index here.");
        ImGui::EndTable();
    }
    ImGui::Separator();
    if (ImGui::BeginTable("hkbBehaviorGraph2", 2, ImGuiTableFlags_Resizable))
    {
        ImGui::TableSetupColumn("bindings", ImGuiTableColumnFlags_WidthFixed, 200.0F);
        ImGui::TableSetupColumn("editarea", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableNextRow();

        ImGui::TableNextColumn();
        objLiveEditList(obj.getByName("bindings"), edit_binding, Hkx::g_def_hkbVariableBindingSet_Binding, file, "bindings",
                        [](auto node) { return node.getByName("memberPath").text().as_string(); });

        ImGui::TableNextColumn();
        if (edit_binding)
        {
            if (ImGui::BeginTable("binding", 2, EditAreaTableFlag))
            {
                stringEdit(edit_binding.getByName("memberPath"),
                           "The memberPath is made up of member names, separated by '/'. Integers after colons in the path are array indices.\n"
                           "For example, \"children:2/blendWeight\" would seek an array member named \"children\", access the second member,\n"
                           "and then look for a member named \"blendWeight\" in that object.");
                if (!strcmp(edit_binding.getByName("bindingType").text().as_string(), "BINDING_TYPE_VARIABLE")) // binding variables
                    linkedPropPickerEdit(edit_binding.getByName("variableIndex"), file, file.m_var_manager,
                                         "The index of the variable that is bound to an object member.");
                else
                    linkedPropPickerEdit(edit_binding.getByName("variableIndex"), file, file.m_prop_manager,
                                         "The index of the variable that is bound to an object member.");
                intScalarEdit(edit_binding.getByName("bitIndex"), file, ImGuiDataType_S8,
                              "The index of the bit to which the variable is bound.\n"
                              "A value of -1 indicates that we are not binding to an individual bit.");
                enumEdit(edit_binding.getByName("bindingType"), Hkx::e_bindingType,
                         "Which data we are binding to.");
                ImGui::EndTable();
            }
        }
        else
            ImGui::TextDisabled("No binding selected.");

        ImGui::EndTable();
    }
}

UICLASS(hkbStateMachine)
{
    if (ImGui::BeginTable("hkbStateMachine1", 2, ImGuiTableFlags_Resizable))
    {
        ImGui::TableSetupColumn("editarea", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("states", ImGuiTableColumnFlags_WidthFixed, 300.0F);
        ImGui::TableNextRow();

        ImGui::TableNextColumn();
        if (ImGui::BeginTable("hkbStateMachine2", 2, EditAreaTableFlag))
        {
            stringEdit(obj.getByName("name"));
            intScalarEdit(obj.getByName("userData"), file, ImGuiDataType_U32);
            refEdit(obj.getByName("variableBindingSet"), {"hkbVariableBindingSet"}, file);

            fullTableSeparator();

            ImGui::TableNextColumn();
            ImGui::BulletText("eventToSendWhenStateOrTransitionChanges");
            addTooltip("If non-null, this event is sent at the beginning and end of a transition, or once for an instantaneous transition.");
            ImGui::TableNextColumn();
            ImGui::Indent(10);
            show_hkbEvent(obj.getByName("eventToSendWhenStateOrTransitionChanges").first_child(), file);
            ImGui::Indent(-10);

            fullTableSeparator();

            refEdit(obj.getByName("startStateChooser"), {"hkbStateChooser"}, file,
                    "an object that chooses the start state"); // this one in my ver replaced by startStateIdSelector
            stateEdit(obj.getByName("startStateId"), file, obj);
            linkedPropPickerEdit(obj.getByName("syncVariableIndex"), file, file.m_var_manager,
                                 "We use variables to sync the start state of the state machine.\n\n"
                                 "The value of this index currently must be initialized to the state id which the user wants the start state to be.\n"
                                 "The state machine syncs it start state by setting the value of the start state to that of the variable during activate\n"
                                 "and sets the variable value to the toState when the transition begins.");
            enumEdit(obj.getByName("startStateMode"), Hkx::e_hkbStateMachine_StartStateMode,
                     "How to set the start state.");

            fullTableSeparator();

            linkedPropPickerEdit(obj.getByName("returnToPreviousStateEventId"), file, file.m_evt_manager,
                                 "If this event is received, the state machine returns to the previous state if there is an appropriate transition defined.");
            linkedPropPickerEdit(obj.getByName("randomTransitionEventId"), file, file.m_evt_manager,
                                 "If this event is received, the state machine chooses a random transition from among those available.\n\n"
                                 "The event of the transition is ignored, but all other considerations of whether the transition should be taken are considered.");
            linkedPropPickerEdit(obj.getByName("transitionToNextHigherStateEventId"), file, file.m_evt_manager,
                                 "If the event is received, the state machine chooses a state with the id higher than the m_currentStateId and do a transition to that state.\n\n"
                                 "The event of the transition is ignored, but all other considerations of whether the transition should be taken are considered.\n"
                                 "If no appropriate transition is found, an immediate transition will occur.");
            linkedPropPickerEdit(obj.getByName("transitionToNextLowerStateEventId"), file, file.m_evt_manager,
                                 "If the event is received, the state machine chooses a state with the id lower than the m_currentStateId and do a transition to that state.\n\n"
                                 "The event of the transition is ignored, but all other considerations of whether the transition should be taken are considered.\n"
                                 "If no appropriate transition is found, an immediate transition will occur.");

            fullTableSeparator();

            refEdit(obj.getByName("wildcardTransitions"), {"hkbStateMachineTransitionInfoArray"}, file,
                    "the list of transitions from any state (don't have a specific from state)");

            fullTableSeparator();

            boolEdit(obj.getByName("wrapAroundStateId"), file,
                     "This decides whether to transition when the m_currentStateId is maximum and m_transitionToNextHigherStateEventId is set\n"
                     "m_currentStateId is minimum and m_transitionToNextLowerStateEventId is set.\n\n"
                     "If this is set to false there would be no transition.\n"
                     "Otherwise there would be a transition but in the first case the next state id will be a lower than the current one\n"
                     "and in the second case the next state id would be higher than the current state id.");
            intScalarEdit(obj.getByName("maxSimultaneousTransitions"), file, ImGuiDataType_S8,
                          "The number of transitions that can be active at once.\n\n"
                          "When a transition B interrupts another transition A, the state machine can continue playing A an input to B.\n"
                          "If a transition is triggered when there are already m_maxSimultaneousTransitions active transitions,\n"
                          "then the oldest transition will be immediately ended to make room for a new one.");
            enumEdit(obj.getByName("selfTransitionMode"), Hkx::e_hkbStateMachine_StateMachineSelfTransitionMode,
                     "How to deal with self-transitions (when the state machine is transitioned to while still active).");

            ImGui::EndTable();
        }

        ImGui::TableNextColumn();
        refList(obj.getByName("states"), {"hkbStateMachineStateInfo"}, file, "stateId", "name", "States");

        ImGui::EndTable();
    }
}

UICLASS(hkbStateMachineStateInfo)
{
    if (ImGui::BeginTable("hkbStateMachineStateInfo1", 4, EditAreaTableFlag))
    {
        auto& file = Hkx::HkxFileManager::getSingleton()->getCurrentFile();

        stringEdit(obj.getByName("name"));
        refEdit(obj.getByName("variableBindingSet"), {"hkbVariableBindingSet"}, file);
        intScalarEdit(obj.getByName("stateId"), file);
        sliderFloatEdit(obj.getByName("probability"), 0.0f, 1.0f, file,
                        "The state probability.  When choosing a random start state, each state is weighted according to its probability.\n"
                        "The probabilities of all of the states being considered are normalized so that their sum is 1.\n"
                        "When choosing a random transition, each transition in weighted according to the probability of the to-state.");
        boolEdit(obj.getByName("enable"), file,
                 "Enable this state. Otherwise, it will be inaccessible.");
        // listeners seems unused

        fullTableSeparator();

        refEdit(obj.getByName("generator"), std::vector(Hkx::g_class_generators.begin(), Hkx::g_class_generators.end()), file,
                "The generator associated with this state.");
        refEdit(obj.getByName("transitions"), {"hkbStateMachineTransitionInfoArray"}, file,
                "The transitions out of this state.");
        fullTableSeparator();

        auto enter_events = obj.getByName("enterNotifyEvents");
        refEdit(enter_events, {"hkbStateMachineEventPropertyArray"}, file,
                "These events are sent when the state is entered.");
        auto exit_events = obj.getByName("exitNotifyEvents");
        refEdit(exit_events, {"hkbStateMachineEventPropertyArray"}, file,
                "These events are sent when the state is exited.");

        if (auto node = file.getObj(enter_events.text().as_string()); node)
        {
            ImGui::TableNextColumn();
            ImGui::PushID(0);
            show_hkbStateMachineEventPropertyArray(node, file);
            ImGui::PopID();
            ImGui::TableNextColumn();
        }

        if (auto node = file.getObj(exit_events.text().as_string()); node)
        {
            ImGui::TableNextColumn();
            ImGui::PushID(1);
            show_hkbStateMachineEventPropertyArray(node, file);
            ImGui::PopID();
            ImGui::TableNextColumn();
        }

        ImGui::EndTable();
    }
}

UICLASS(hkbStateMachineTransitionInfoArray)
{
    static pugi::xml_node obj_cache;
    static pugi::xml_node edit_trans = {};
    if (obj_cache != obj)
        edit_trans = {};
    obj_cache = obj; // workaround to prevent crashing between files, asserting that may only be one menu at the same time

    if (ImGui::BeginTable("hkbStateMachineTransitionInfoArray1", 2, ImGuiTableFlags_Resizable))
    {
        ImGui::TableSetupColumn("trans", ImGuiTableColumnFlags_WidthFixed, 200.0F);
        ImGui::TableSetupColumn("editarea", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextColumn();
        auto transitions_node = obj.getByName("transitions");
        auto state_machine    = getParentStateMachine(obj, file);

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("transitions"), ImGui::SameLine();
        if (ImGui::Button(ICON_FA_PLUS_CIRCLE))
            appendXmlString(transitions_node, Hkx::g_def_hkbStateMachine_TransitionInfo);
        addTooltip("Add new transition");
        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(transitions_node.attribute("numelements").as_string());

        constexpr auto table_flag = ImGuiTableFlags_SizingFixedFit |
            ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
            ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_NoBordersInBody;
        if (ImGui::BeginTable("transitions", 2, table_flag))
        {
            pugi::xml_node mark_delete = {};
            bool           do_delete   = false;
            auto           numelements = transitions_node.attribute("numelements").as_ullong();
            size_t         i           = 0;
            for (auto transition : transitions_node.children())
            {
                ImGui::PushID(i);

                ImGui::TableNextColumn();
                if (ImGui::Button(ICON_FA_MINUS_CIRCLE))
                {
                    do_delete   = true;
                    mark_delete = transition;
                }
                addTooltip("Remove");

                ImGui::TableNextColumn();
                bool selected    = edit_trans == transition;
                auto to_state_id = transition.getByName("toStateId").text().as_int();
                auto to_state    = getStateById(state_machine, to_state_id, file);
                if (ImGui::Selectable(std::format("To: {} - {}", to_state_id, to_state.getByName("name").text().as_string()).c_str(),
                                      &selected))
                    edit_trans = transition;

                ImGui::PopID();

                ++i;
            }
            if (do_delete)
            {
                if (edit_trans == mark_delete)
                    edit_trans = {};
                transitions_node.remove_child(mark_delete);
                transitions_node.attribute("numelements") = transitions_node.attribute("numelements").as_int() - 1;
            }

            ImGui::EndTable();
        }

        ImGui::TableNextColumn();
        if (edit_trans)
        {
            if (ImGui::BeginTable("hkbStateMachineTransitionInfoArray2", 2, EditAreaTableFlag))
            {
                show_hkbStateMachine_TransitionInfo(edit_trans, file);
                ImGui::EndTable();
            }
        }
        else
            ImGui::TextDisabled("Not editing any transition");

        ImGui::EndTable();
    }
}

UICLASS(hkbBlendingTransitionEffect)
{
    if (ImGui::BeginTable("hkbBlendingTransitionEffect", 4, EditAreaTableFlag))
    {
        stringEdit(obj.getByName("name"));
        intScalarEdit(obj.getByName("userData"), file, ImGuiDataType_U32);
        refEdit(obj.getByName("variableBindingSet"), {"hkbVariableBindingSet"}, file);

        fullTableSeparator();

        enumEdit(obj.getByName("selfTransitionMode"), Hkx::e_hkbTransitionEffect_SelfTransitionMode,
                 "What to do if the to-generator is already active when the transition activates it.");
        enumEdit(obj.getByName("eventMode"), Hkx::e_hkbTransitionEffect_EventMode,
                 "How to process the events of the from- and to-generators.");

        fullTableSeparator();

        sliderFloatEdit(obj.getByName("duration"), 0.0f, 1000.0f, file,
                        "The duration of the transition.");
        sliderFloatEdit(obj.getByName("toGeneratorStartTimeFraction"), 0.0f, 1.0f, file,
                        "The start time of the to-generator when the transition begins, expressed as a fraction of its duration.");

        fullTableSeparator();

        enumEdit(obj.getByName("endMode"), Hkx::e_hkbBlendingTransitionEffect_EndMode,
                 "The treatment of the end of the from-generator.");
        enumEdit(obj.getByName("blendCurve"), Hkx::e_hkbBlendCurveUtils_BlendCurve,
                 "Which blend curve to use.");
        flagEdit(obj.getByName("flags"), Hkx::f_hkbBlendingTransitionEffect_FlagBits,
                 "Flags to indicate specialized behavior.");
    }
}

UICLASS(hkbBlenderGeneratorChild)
{
    if (ImGui::BeginTable("hkbBlenderGeneratorChild", 2, EditAreaTableFlag))
    {
        refEdit(obj.getByName("variableBindingSet"), {"hkbVariableBindingSet"}, file);

        fullTableSeparator();

        refEdit(obj.getByName("generator"), std::vector(Hkx::g_class_generators.begin(), Hkx::g_class_generators.end()), file,
                "The generator associated with this child.");
        refEdit(obj.getByName("boneWeights"), {"hkbBoneWeightArray"}, file,
                "A weight for each bone");
        floatEdit(obj.getByName("weight"), file,
                  "The blend weight for this child.");
        floatEdit(obj.getByName("worldFromModelWeight"), file,
                  "The weights used to determine the contribution of this child's world-from-model transform to the final world-from-model.");

        ImGui::EndTable();
    }
}

UICLASS(hkbBlenderGenerator)
{
    static pugi::xml_node obj_cache;
    static pugi::xml_node edit_child = {};
    if (obj_cache != obj)
        edit_child = {};
    obj_cache = obj; // workaround to prevent crashing between files, asserting that may only be one menu at the same time

    if (ImGui::BeginTable("hkbBlenderGenerator1", 2, ImGuiTableFlags_Resizable))
    {
        ImGui::TableSetupColumn("children", ImGuiTableColumnFlags_WidthFixed, 300.0F);
        ImGui::TableSetupColumn("editarea", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableNextRow();

        ImGui::TableNextColumn();
        auto result = refLiveEditList(obj.getByName("children"), {"hkbBlenderGeneratorChild"}, file, "", "", "Children");
        if (result)
            edit_child = result;

        ImGui::TableNextColumn();
        if (ImGui::BeginTable("hkbBlenderGenerator2", 2, EditAreaTableFlag))
        {
            stringEdit(obj.getByName("name"));
            intScalarEdit(obj.getByName("userData"), file, ImGuiDataType_U32);
            refEdit(obj.getByName("variableBindingSet"), {"hkbVariableBindingSet"}, file);

            fullTableSeparator();

            floatEdit(obj.getByName("referencePoseWeightThreshold"), file,
                      "If the sum of non-additive generator weights falls below this threshold, the reference pose is blended in.");
            floatEdit(obj.getByName("blendParameter"), file,
                      "This value controls the parametric blend.");
            floatEdit(obj.getByName("minCyclicBlendParameter"), file,
                      "The minimum value the blend parameter can have when doing a cyclic parametric blend.");
            floatEdit(obj.getByName("maxCyclicBlendParameter"), file,
                      "The maimum value the blend parameter can have when doing a cyclic parametric blend.");
            intScalarEdit(obj.getByName("indexOfSyncMasterChild"), file, ImGuiDataType_S16,
                          "If you want a particular child's duration to be used to sync all of the other children, set this to the index of the child.\n"
                          "Otherwise, set it to -1.");
            flagEdit(obj.getByName("flags"), Hkx::f_hkbBlenderGenerator_BlenderFlags,
                     "The flags affecting specialized behavior.", "", true);
            boolEdit(obj.getByName("subtractLastChild"), file,
                     "If this is set to true then the last child will be a subtracted from the blend of the rest");

            ImGui::EndTable();
        }
        if (edit_child)
        {
            ImGui::Separator();
            ImGui::Text("Editing Child %s", hkobj2str(edit_child).c_str());
            show_hkbBlenderGeneratorChild(edit_child, file);
        }

        ImGui::EndTable();
    }
}

UICLASS(BSBoneSwitchGeneratorBoneData)
{
    if (ImGui::BeginTable("BSBoneSwitchGeneratorBoneData", 2, EditAreaTableFlag))
    {
        refEdit(obj.getByName("variableBindingSet"), {"hkbVariableBindingSet"}, file);
        refEdit(obj.getByName("pGenerator"), std::vector(Hkx::g_class_generators.begin(), Hkx::g_class_generators.end()), file);
        refEdit(obj.getByName("spBoneWeight"), {"hkbBoneWeightArray"}, file);

        ImGui::EndTable();
    }
}

UICLASS(BSBoneSwitchGenerator)
{
    static pugi::xml_node obj_cache;
    static pugi::xml_node edit_child = {};
    if (obj_cache != obj)
        edit_child = {};
    obj_cache = obj; // workaround to prevent crashing between files, asserting that may only be one menu at the same time

    if (ImGui::BeginTable("BSBoneSwitchGenerator1", 2, ImGuiTableFlags_Resizable))
    {
        ImGui::TableSetupColumn("children", ImGuiTableColumnFlags_WidthFixed, 300.0F);
        ImGui::TableSetupColumn("editarea", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableNextRow();

        ImGui::TableNextColumn();
        auto result = refLiveEditList(obj.getByName("ChildrenA"), {"hkbBlenderGeneratorChild"}, file, "", "", "Children");
        if (result)
            edit_child = result;

        ImGui::TableNextColumn();
        if (ImGui::BeginTable("BSBoneSwitchGenerator2", 2, EditAreaTableFlag, {-FLT_MIN, 110.0f}))
        {
            stringEdit(obj.getByName("name"));
            intScalarEdit(obj.getByName("userData"), file, ImGuiDataType_U32);
            refEdit(obj.getByName("variableBindingSet"), {"hkbVariableBindingSet"}, file);
            refEdit(obj.getByName("pDefaultGenerator"), {"BSBoneSwitchGeneratorBoneData"}, file);

            ImGui::EndTable();
        }
        if (edit_child)
        {
            ImGui::Separator();
            ImGui::Text("Editing Child %s", hkobj2str(edit_child).c_str());
            show_BSBoneSwitchGeneratorBoneData(edit_child, file);
        }

        ImGui::EndTable();
    }
}

UICLASS(hkbBoneWeightArray)
{
    if (ImGui::BeginTable("hkbBoneWeightArray", 2, EditAreaTableFlag))
    {
        refEdit(obj.getByName("variableBindingSet"), {"hkbVariableBindingSet"}, file);

        auto bone_weights = obj.getByName("boneWeights");

        ImGui::TableNextColumn();
        size_t numelements = bone_weights.attribute("numelements").as_ullong();
        if (ImGui::InputScalar("numelements", ImGuiDataType_U64, &numelements))
            bone_weights.attribute("numelements") = numelements;

        ImGui::TableNextColumn();

        ImGui::TableNextColumn();
        std::string value = bone_weights.text().as_string();
        if (ImGui::InputTextMultiline(bone_weights.attribute("name").as_string(), &value))
            bone_weights.text() = value.c_str();
        addTooltip("A weight for each bone.\nIf the list is too short, missing bones are assumed to have weight 1.");

        ImGui::TableNextColumn();

        ImGui::EndTable();
    }
}

UICLASS(hkbBoneIndexArray)
{
    if (ImGui::BeginTable("hkbBoneIndexArray", 2, EditAreaTableFlag))
    {
        refEdit(obj.getByName("variableBindingSet"), {"hkbVariableBindingSet"}, file);

        auto bone_indices = obj.getByName("boneIndices");

        ImGui::TableNextColumn();
        size_t numelements = bone_indices.attribute("numelements").as_ullong();
        if (ImGui::InputScalar("numelements", ImGuiDataType_U64, &numelements))
            bone_indices.attribute("numelements") = numelements;

        ImGui::TableNextColumn();

        ImGui::TableNextColumn();
        std::string value = bone_indices.text().as_string();
        if (ImGui::InputTextMultiline(bone_indices.attribute("name").as_string(), &value))
            bone_indices.text() = value.c_str();
        addTooltip("An array of bone indices.");

        ImGui::TableNextColumn();

        ImGui::EndTable();
    }
}

UICLASS(hkbClipTriggerArray)
{
    static pugi::xml_node obj_cache;
    static pugi::xml_node edit_trigger = {};
    if (obj_cache != obj)
        edit_trigger = {};
    obj_cache = obj; // workaround to prevent crashing between files, asserting that may only be one menu at the same time

    if (ImGui::BeginTable("hkbClipTriggerArray1", 2, ImGuiTableFlags_Resizable))
    {
        ImGui::TableSetupColumn("triggers", ImGuiTableColumnFlags_WidthFixed, 200.0F);
        ImGui::TableSetupColumn("editarea", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextColumn();
        auto triggers_node = obj.getByName("triggers");

        ImGui::AlignTextToFramePadding();

        objLiveEditList(obj.getByName("triggers"), edit_trigger, Hkx::g_def_hkbClipTrigger, file, "triggers",
                        [&file](auto node) { return file.m_evt_manager.getEntry(node.getByName("event").first_child().getByName("id").text().as_uint()).getName(); });

        ImGui::TableNextColumn();
        if (edit_trigger)
        {
            if (ImGui::BeginTable("hkbClipTriggerArray2", 2, EditAreaTableFlag))
            {
                floatEdit(edit_trigger.getByName("localTime"), file,
                          "The local time at which the trigger is attached.\n\n"
                          "This time is relative to the remaining portion of the clip after cropping.");

                ImGui::TableNextColumn();
                ImGui::BulletText("event");
                addTooltip("The event to send when the time comes.");
                ImGui::TableNextColumn();

                ImGui::Indent(10);
                show_hkbEvent(edit_trigger.getByName("event").first_child(), file);
                ImGui::Indent(-10);

                boolEdit(edit_trigger.getByName("relativeToEndOfClip"), file,
                         "Whether m_localTime is relative to the end of the clip.\n\n"
                         "If false, m_localTime should be positive or zero.\n"
                         "If true, m_localTime should be negative or zero.");
                boolEdit(edit_trigger.getByName("acyclic"), file,
                         "Whether the trigger is a cyclic or acyclic.\n\n"
                         "m_acyclic and m_relativeToEndOfClip are mutually exclusive.");
                boolEdit(edit_trigger.getByName("isAnnotation"), file,
                         "Whether or not this trigger was converted from an annotation attached to the animation.");

                ImGui::EndTable();
            }
        }
        else
            ImGui::TextDisabled("Not editing any trigger");

        ImGui::EndTable();
    }
}

UICLASS(hkbClipGenerator)
{
    if (ImGui::BeginTable("hkbClipGenerator", 4, EditAreaTableFlag, {-FLT_MIN, 200.0f}))
    {
        stringEdit(obj.getByName("name"));
        intScalarEdit(obj.getByName("userData"), file, ImGuiDataType_U32);
        refEdit(obj.getByName("variableBindingSet"), {"hkbVariableBindingSet"}, file);
        stringEdit(obj.getByName("animationName"),
                   "The name of the animation to play.");

        fullTableSeparator();

        floatEdit(obj.getByName("cropStartAmountLocalTime"), file,
                  "The number of seconds (in clip time) to crop the beginning of the clip.");
        floatEdit(obj.getByName("cropEndAmountLocalTime"), file,
                  "The number of seconds (in clip time) to crop the end of the clip.");
        floatEdit(obj.getByName("startTime"), file,
                  "The time at which to start the animation in local time.");
        floatEdit(obj.getByName("playbackSpeed"), file,
                  "Playback speed (negative for backward).");
        floatEdit(obj.getByName("enforcedDuration"), file,
                  "If m_enforcedDuration is greater than zero, the clip will be scaled to have the enforced duration.\n\n"
                  "Note that m_playbackSpeed can still be used.\n"
                  "The duration will only match the enforced duration when m_playbackSpeed is 1.\n"
                  "The effective duration will be m_enforcedDuration / m_playbackSpeed.");
        sliderFloatEdit(obj.getByName("userControlledTimeFraction"), 0.0f, 1.0f, file,
                        "In user controlled mode, this fraction (between 0 and 1) dictates the time of the animation.\n\n"
                        "This value interpolates along the clip within the part that remains after cropping.");

        fullTableSeparator();

        intScalarEdit(obj.getByName("animationBindingIndex"), file, ImGuiDataType_S16,
                      "An index into the character's hkbAnimationBindingSet.");
        enumEdit(obj.getByName("mode"), Hkx::e_hkbClipGenerator_PlaybackMode,
                 "The playback mode.");
        flagEdit(obj.getByName("flags"), Hkx::f_hkbClipGenerator_ClipFlags,
                 "Flags for specialized behavior.");

        refEdit(obj.getByName("triggers"), {"hkbClipTriggerArray"}, file,
                "Triggers (events that occur at specific times).");

        ImGui::EndTable();
    }

    auto trigger_obj = file.getObj(obj.getByName("triggers").text().as_string());
    if (trigger_obj)
    {
        ImGui::Separator();
        ImGui::TextUnformatted("Edit triggers");
        show_hkbClipTriggerArray(trigger_obj, file);
    }
}

UICLASS(BSSynchronizedClipGenerator)
{
    if (ImGui::BeginTable("BSSynchronizedClipGenerator", 4, EditAreaTableFlag))
    {
        stringEdit(obj.getByName("name"));
        intScalarEdit(obj.getByName("userData"), file, ImGuiDataType_U32);
        refEdit(obj.getByName("variableBindingSet"), {"hkbVariableBindingSet"}, file);
        intScalarEdit(obj.getByName("sAnimationBindingIndex"), file, ImGuiDataType_S16);

        fullTableSeparator();

        refEdit(obj.getByName("pClipGenerator"), {"hkbClipGenerator"}, file);
        stringEdit(obj.getByName("SyncAnimPrefix"));

        fullTableSeparator();

        floatEdit(obj.getByName("fGetToMarkTime"), file);
        floatEdit(obj.getByName("fMarkErrorThreshold"), file);

        fullTableSeparator();

        boolEdit(obj.getByName("bSyncClipIgnoreMarkPlacement"), file);
        boolEdit(obj.getByName("bLeadCharacter"), file);
        boolEdit(obj.getByName("bReorientSupportChar"), file);
        boolEdit(obj.getByName("bApplyMotionFromRoot"), file);

        ImGui::EndTable();
    }
}

UICLASS(hkbManualSelectorGenerator)
{
    if (ImGui::BeginTable("hkbManualSelectorGenerator1", 2, ImGuiTableFlags_Resizable))
    {
        ImGui::TableSetupColumn("editarea", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("gens", ImGuiTableColumnFlags_WidthFixed, 300.0F);
        ImGui::TableNextRow();

        ImGui::TableNextColumn();
        if (ImGui::BeginTable("hkbManualSelectorGenerator2", 2, EditAreaTableFlag))
        {
            stringEdit(obj.getByName("name"));
            intScalarEdit(obj.getByName("userData"), file, ImGuiDataType_U32);
            refEdit(obj.getByName("variableBindingSet"), {"hkbVariableBindingSet"}, file);

            fullTableSeparator();

            intScalarEdit(obj.getByName("selectedGeneratorIndex"), file, ImGuiDataType_S16,
                          "use this to select a generator from the above list");
            intScalarEdit(obj.getByName("currentGeneratorIndex"), file, ImGuiDataType_S16,
                          "the current generator that has been selected");

            ImGui::EndTable();
        }

        ImGui::TableNextColumn();
        refList(obj.getByName("generators"), {Hkx::g_class_generators.begin(), Hkx::g_class_generators.end()}, file, {}, "name", "generators");

        ImGui::EndTable();
    }
}

UICLASS(hkbModifierGenerator)
{
    if (ImGui::BeginTable("hkbModifierGenerator", 4, EditAreaTableFlag))
    {
        stringEdit(obj.getByName("name"));
        intScalarEdit(obj.getByName("userData"), file, ImGuiDataType_U32);
        refEdit(obj.getByName("variableBindingSet"), {"hkbVariableBindingSet"}, file);

        fullTableSeparator();

        refEdit(obj.getByName("modifier"), {Hkx::g_class_modifiers.begin(), Hkx::g_class_modifiers.end()}, file,
                "The modifier being applied to a generator.");
        refEdit(obj.getByName("generator"), {Hkx::g_class_generators.begin(), Hkx::g_class_generators.end()}, file,
                "The generator to which a modifier is being applied.");

        ImGui::EndTable();
    }
}

UICLASS(hkbBehaviorReferenceGenerator)
{
    if (ImGui::BeginTable("hkbBehaviorReferenceGenerator", 4, EditAreaTableFlag))
    {
        stringEdit(obj.getByName("name"));
        intScalarEdit(obj.getByName("userData"), file, ImGuiDataType_U32);
        refEdit(obj.getByName("variableBindingSet"), {"hkbVariableBindingSet"}, file);
        stringEdit(obj.getByName("behaviorName"),
                   "the name of the behavior");

        ImGui::EndTable();
    }
}

UICLASS(hkbPoseMatchingGenerator)
{
    if (ImGui::BeginTable("hkbPoseMatchingGenerator", 4, EditAreaTableFlag, {-FLT_MIN, 27 * 6}))
    {
        quadEdit(obj.getByName("worldFromModelRotation"), file,
                 "The rotation of the frame of reference used for pose matching.");
        enumEdit(obj.getByName("mode"), Hkx::e_hkbPoseMatchingGenerator_Mode,
                 "Whether matching poses or playing out the remaining animation.");

        fullTableSeparator();

        sliderFloatEdit(obj.getByName("blendSpeed"), 0.0001f, 1000.0f, file,
                        "Speed at which to blend in and out the blend weights (in weight units per second).");
        sliderFloatEdit(obj.getByName("minSpeedToSwitch"), 0.0f, 100.0f, file,
                        "Don't switch if the pelvis speed is below this.");
        sliderFloatEdit(obj.getByName("minSwitchTimeNoError"), 0.0f, 100.0f, file,
                        "The minimum time between switching matches when the matching error is 0.\n\n"
                        "In order to avoid too much oscillating back and forth between matches, we introduce some hysteresis.\n"
                        "We only allow a switch to a new match after a minimum amount of time has passed without a better match being found.\n"
                        "The time bound is a linear function of the error.");
        sliderFloatEdit(obj.getByName("minSwitchTimeFullError"), 0.0f, 100.0f, file,
                        "The minimum time between switching matches when the matching error is 1.");

        fullTableSeparator();

        linkedPropPickerEdit(obj.getByName("startPlayingEventId"), file, file.m_evt_manager,
                             "An event to set m_mode to MODE_PLAY.");
        linkedPropPickerEdit(obj.getByName("startMatchingEventId"), file, file.m_evt_manager,
                             "An event to set m_mode to MODE_MATCH.");

        fullTableSeparator();

        intScalarEdit(obj.getByName("rootBoneIndex"), file, ImGuiDataType_S16,
                      "The root (ragdoll) bone used for pose matching. If this is -1, the index is taken from the character's hkbBoneInfo.");
        intScalarEdit(obj.getByName("otherBoneIndex"), file, ImGuiDataType_S16,
                      "A second (ragdoll) bone used for pose matching. If this is -1, the index is taken from the character's hkbBoneInfo.");
        intScalarEdit(obj.getByName("anotherBoneIndex"), file, ImGuiDataType_S16,
                      "A third (ragdoll) bone used for pose matching. If this is -1, the index is taken from the character's hkbBoneInfo.");
        intScalarEdit(obj.getByName("pelvisIndex"), file, ImGuiDataType_S16,
                      "The (ragdoll) pelvis bone used to measure the speed of the ragdoll.");

        ImGui::EndTable();
    }
    ImGui::Separator();
    show_hkbBlenderGenerator(obj, file);
}

UICLASS(BSCyclicBlendTransitionGenerator)
{
    if (ImGui::BeginTable("BSCyclicBlendTransitionGenerator", 4, EditAreaTableFlag))
    {
        stringEdit(obj.getByName("name"));
        intScalarEdit(obj.getByName("userData"), file, ImGuiDataType_U32);
        refEdit(obj.getByName("variableBindingSet"), {"hkbVariableBindingSet"}, file);
        refEdit(obj.getByName("pBlenderGenerator"), {"hkbBlenderGenerator", "hkbPoseMatchingGenerator"}, file);

        fullTableSeparator();

        ImGui::TableNextColumn();
        ImGui::BulletText("EventToFreezeBlendValue");
        ImGui::TableNextColumn();

        ImGui::TableNextColumn();

        ImGui::BulletText("EventToCrossBlend");
        ImGui::TableNextColumn();

        ImGui::TableNextColumn();
        if (ImGui::BeginTable("BSCyclicBlendTransitionGenerator1", 2, EditAreaTableFlag))
        {
            show_hkbEvent(obj.getByName("EventToFreezeBlendValue").first_child(), file);
            ImGui::EndTable();
        }
        ImGui::TableNextColumn();

        ImGui::TableNextColumn();
        if (ImGui::BeginTable("BSCyclicBlendTransitionGenerator2", 2, EditAreaTableFlag))
        {
            show_hkbEvent(obj.getByName("EventToCrossBlend").first_child(), file);
            ImGui::EndTable();
        }
        ImGui::TableNextColumn();

        fullTableSeparator();

        floatEdit(obj.getByName("fBlendParameter"), file);
        floatEdit(obj.getByName("fTransitionDuration"), file);
        enumEdit(obj.getByName("eBlendCurve"), Hkx::e_hkbBlendCurveUtils_BlendCurve);

        ImGui::EndTable();
    }
}

UICLASS(BSiStateTaggingGenerator)
{
    if (ImGui::BeginTable("BSiStateTaggingGenerator", 4, EditAreaTableFlag))
    {
        stringEdit(obj.getByName("name"));
        intScalarEdit(obj.getByName("userData"), file, ImGuiDataType_U32);
        refEdit(obj.getByName("variableBindingSet"), {"hkbVariableBindingSet"}, file);
        refEdit(obj.getByName("pDefaultGenerator"), {Hkx::g_class_generators.begin(), Hkx::g_class_generators.end()}, file);

        fullTableSeparator();

        intScalarEdit(obj.getByName("iStateToSetAs"), file);
        intScalarEdit(obj.getByName("iPriority"), file);

        ImGui::EndTable();
    }
}

UICLASS(BSOffsetAnimationGenerator)
{
    if (ImGui::BeginTable("BSOffsetAnimationGenerator", 4, EditAreaTableFlag))
    {
        stringEdit(obj.getByName("name"));
        intScalarEdit(obj.getByName("userData"), file, ImGuiDataType_U32);
        refEdit(obj.getByName("variableBindingSet"), {"hkbVariableBindingSet"}, file);

        fullTableSeparator();

        refEdit(obj.getByName("pDefaultGenerator"), {Hkx::g_class_generators.begin(), Hkx::g_class_generators.end()}, file);
        refEdit(obj.getByName("pOffsetClipGenerator"), {"hkbClipGenerator"}, file);

        fullTableSeparator();

        floatEdit(obj.getByName("fOffsetVariable"), file);
        floatEdit(obj.getByName("fOffsetRangeStart"), file);
        floatEdit(obj.getByName("fOffsetRangeEnd"), file);

        ImGui::EndTable();
    }
}

UICLASS(hkbModifierList)
{
    if (ImGui::BeginTable("hkbModifierList1", 2, ImGuiTableFlags_Resizable))
    {
        ImGui::TableSetupColumn("editarea", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("mods", ImGuiTableColumnFlags_WidthFixed, 300.0F);
        ImGui::TableNextRow();

        ImGui::TableNextColumn();
        if (ImGui::BeginTable("hkbModifierList2", 2, EditAreaTableFlag))
        {
            show_hkbModifier(obj, file);
            ImGui::EndTable();
        }

        ImGui::TableNextColumn();
        refList(obj.getByName("modifiers"), {Hkx::g_class_modifiers.begin(), Hkx::g_class_modifiers.end()}, file, {}, "name", "modifiers");

        ImGui::EndTable();
    }
}

UICLASS(hkbExpressionDataArray)
{
    static pugi::xml_node obj_cache;
    static pugi::xml_node edit_expr = {};
    if (obj_cache != obj)
        edit_expr = {};
    obj_cache = obj; // workaround to prevent crashing between files, asserting that may only be one menu at the same time

    if (ImGui::BeginTable("hkbExpressionDataArray1", 2, ImGuiTableFlags_Resizable))
    {
        ImGui::TableSetupColumn("exprs", ImGuiTableColumnFlags_WidthFixed, 200.0F);
        ImGui::TableSetupColumn("editarea", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextColumn();

        objLiveEditList(obj.getByName("expressionsData"), edit_expr, Hkx::g_def_hkbExpressionData, file, "expressions",
                        [](auto node) { return node.getByName("expression").text().as_string(); });

        ImGui::TableNextColumn();
        if (edit_expr)
        {
            if (ImGui::BeginTable("hkbExpressionDataArray2", 2, EditAreaTableFlag))
            {
                stringEdit(edit_expr.getByName("expression"),
                           "Expression that's input by the user in the form: \"variablename = expression\" or \"eventname = boolean expression\".");
                linkedPropPickerEdit(edit_expr.getByName("assignmentVariableIndex"), file, file.m_var_manager,
                                     "This is the variable that we will assign result to (-1 if not found). For internal use.");
                linkedPropPickerEdit(edit_expr.getByName("assignmentEventIndex"), file, file.m_evt_manager,
                                     "This is the event we will raise if result > 0 (-1 if not found). For internal use.");
                enumEdit(edit_expr.getByName("eventMode"), Hkx::e_hkbExpressionData_ExpressionEventMode,
                         "Under what circumstances to send the event.");

                ImGui::EndTable();
            }
        }
        else
            ImGui::TextDisabled("Not editing any expression");

        ImGui::EndTable();
    }
}

UICLASS(hkbEvaluateExpressionModifier)
{
    if (ImGui::BeginTable("hkbEvaluateExpressionModifier", 4, EditAreaTableFlag, {-FLT_MIN, 27 * 3}))
    {
        show_hkbModifier(obj, file);

        fullTableSeparator();

        refEdit(obj.getByName("expressions"), {"hkbExpressionDataArray"}, file,
                "A set of expressions to be evaluated.");

        ImGui::EndTable();
    }
    if (auto exprs = file.getObj(obj.getByName("expressions").text().as_string()); exprs)
    {
        ImGui::Separator();
        show_hkbExpressionDataArray(exprs, file);
    }
}

UICLASS(hkbEventDrivenModifier)
{
    if (ImGui::BeginTable("hkbEventDrivenModifier1", 4, EditAreaTableFlag, {-FLT_MIN, 27 * 2}))
    {
        show_hkbModifier(obj, file);
        ImGui::EndTable();
    }
    ImGui::Separator();
    if (ImGui::BeginTable("hkbEventDrivenModifier2", 2, EditAreaTableFlag))
    {
        refEdit(obj.getByName("modifier"), {Hkx::g_class_modifiers.begin(), Hkx::g_class_modifiers.end()}, file,
                "The nested modifier that is to be wrapped.");

        fullTableSeparator();

        linkedPropPickerEdit(obj.getByName("activateEventId"), file, file.m_evt_manager,
                             "Event used to activate the wrapped modifier");
        linkedPropPickerEdit(obj.getByName("deactivateEventId"), file, file.m_evt_manager,
                             "Event used to deactivate the wrapped modifier");
        boolEdit(obj.getByName("activeByDefault"), file);

        ImGui::EndTable();
    }
}

UICLASS(hkbEventRangeDataArray)
{
    static pugi::xml_node obj_cache;
    static pugi::xml_node edit_range = {};
    if (obj_cache != obj)
        edit_range = {};
    obj_cache = obj; // workaround to prevent crashing between files, asserting that may only be one menu at the same time

    if (ImGui::BeginTable("hkbEventRangeDataArray1", 2, ImGuiTableFlags_Resizable))
    {
        ImGui::TableSetupColumn("ranges", ImGuiTableColumnFlags_WidthFixed, 200.0F);
        ImGui::TableSetupColumn("editarea", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextColumn();

        objLiveEditList(obj.getByName("eventData"), edit_range, Hkx::g_def_hkbEventRangeData, file, "ranges",
                        [&file](auto node) { return fmt::format("{} {}",
                                                                node.getByName("upperBound").text().as_float(),
                                                                file.m_evt_manager.getEntry(node.getByName("event").first_child().getByName("id").text().as_int()).getName()); });

        ImGui::TableNextColumn();
        if (edit_range)
        {
            if (ImGui::BeginTable("hkbEventRangeDataArray2", 2, EditAreaTableFlag))
            {
                floatEdit(edit_range.getByName("upperBound"), file,
                          "The highest value in this range.  The lowest value of this range is the upperBound from the previous range.");
                enumEdit(edit_range.getByName("eventMode"), Hkx::e_hkbEventRangeData_EventRangeMode,
                         "Under what circumstances to send the event.");

                fullTableSeparator();

                ImGui::TableNextColumn();
                ImGui::Text("event");
                ImGui::TableNextColumn();

                ImGui::Indent(10);
                show_hkbEvent(edit_range.getByName("event").first_child(), file);
                ImGui::Indent(-10);

                ImGui::EndTable();
            }
        }
        else
            ImGui::TextDisabled("Not editing any range");

        ImGui::EndTable();
    }
}

UICLASS(hkbEventsFromRangeModifier)
{
    if (ImGui::BeginTable("hkbEventDrivenModifier1", 4, EditAreaTableFlag, {-FLT_MIN, 27 * 4}))
    {
        show_hkbModifier(obj, file);

        fullTableSeparator();

        floatEdit(obj.getByName("inputValue"), file,
                  "The value that is checked against the ranges to decide which events should be sent.");
        floatEdit(obj.getByName("lowerBound"), file,
                  "A lower bound of all of the range intervals.");

        fullTableSeparator();

        refEdit(obj.getByName("eventRanges"), {"hkbEventRangeDataArray"}, file,
                "A series of intervals, each of which has an event associated with it. Note that these must be in increasing order.");

        ImGui::EndTable();
    }
    if (auto ranges = file.getObj(obj.getByName("eventRanges").text().as_string()); ranges)
    {
        ImGui::Separator();
        show_hkbEventRangeDataArray(ranges, file);
    }
}

UICLASS(hkbGetUpModifier)
{
    if (ImGui::BeginTable("hkbGetUpModifier1", 4, EditAreaTableFlag, {-FLT_MIN, 27 * 2}))
    {
        show_hkbModifier(obj, file);
        ImGui::EndTable();
    }
    ImGui::Separator();
    if (ImGui::BeginTable("hkbGetUpModifier2", 2, EditAreaTableFlag, {-FLT_MIN, 28}))
    {
        quadEdit(obj.getByName("groundNormal"), file,
                 "The character's up-vector is aligned with this vector until m_alignWithGroundDuration elapses, after which it is aligned with the world up-vector.");
        ImGui::EndTable();
    }
    ImGui::Separator();
    if (ImGui::BeginTable("hkbGetUpModifier3", 4, EditAreaTableFlag))
    {
        sliderFloatEdit(obj.getByName("duration"), 0.0f, 100.0f, file,
                        "Duration for aligning the character's up-vector.");
        sliderFloatEdit(obj.getByName("alignWithGroundDuration"), 0.0f, 1000.0f, file,
                        "Duration for which the character's up-vector is aligned with m_groundNormal, after which it is aligned with the world up-vector.");

        fullTableSeparator();

        intScalarEdit(obj.getByName("rootBoneIndex"), file, ImGuiDataType_S16,
                      "The root (ragdoll) bone used for pose matching. If this is -1, the index is taken from the character's hkbBoneInfo.");
        intScalarEdit(obj.getByName("otherBoneIndex"), file, ImGuiDataType_S16,
                      "A second (ragdoll) bone used for pose matching. If this is -1, the index is taken from the character's hkbBoneInfo.");
        intScalarEdit(obj.getByName("anotherBoneIndex"), file, ImGuiDataType_S16,
                      "A third (ragdoll) bone used for pose matching. If this is -1, the index is taken from the character's hkbBoneInfo.");

        ImGui::EndTable();
    }
}

UICLASS(hkbKeyframeBonesModifier)
{
    if (ImGui::BeginTable("hkbKeyframeBonesModifier1", 4, EditAreaTableFlag, {-FLT_MIN, 27 * 3 + 2}))
    {
        show_hkbModifier(obj, file);

        fullTableSeparator();

        refEdit(obj.getByName("keyframedBonesList"), {"hkbBoneIndexArray"}, file,
                "Keyframed bone list that HAT can set.");

        ImGui::EndTable();
    }
    ImGui::Separator();

    static pugi::xml_node obj_cache;
    static pugi::xml_node edit_info = {};
    if (obj_cache != obj)
        edit_info = {};
    obj_cache = obj; // workaround to prevent crashing between files, asserting that may only be one menu at the same time

    if (ImGui::BeginTable("hkbKeyframeBonesModifier2", 2, ImGuiTableFlags_Resizable))
    {
        ImGui::TableSetupColumn("infos", ImGuiTableColumnFlags_WidthFixed, 200.0F);
        ImGui::TableSetupColumn("editarea", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextColumn();

        objLiveEditList(obj.getByName("keyframeInfo"), edit_info, Hkx::g_def_hkbKeyframeBonesModifier_KeyframeInfo, file, "keyframeInfo",
                        [](auto node) { return node.getByName("boneIndex").text().as_string(); });

        ImGui::TableNextColumn();
        if (edit_info)
        {
            if (ImGui::BeginTable("hkbKeyframeBonesModifier3", 2, EditAreaTableFlag))
            {
                quadEdit(edit_info.getByName("keyframedPosition"), file,
                         "The position of the keyframed bone.");
                quadEdit(edit_info.getByName("keyframedRotation"), file,
                         "The orientation of the keyframed bone.");
                intScalarEdit(edit_info.getByName("boneIndex"), file, ImGuiDataType_S16,
                              "The ragdoll bone to be keyframed.");
                boolEdit(edit_info.getByName("isValid"), file,
                         "Whether or not m_keyframedPosition and m_keyframedRotation are valid.");

                ImGui::EndTable();
            }
        }
        else
            ImGui::TextDisabled("Not editing any info");

        ImGui::EndTable();
    }
}

UICLASS(hkbPoweredRagdollControlsModifier)
{
    if (ImGui::BeginTable("hkbPoweredRagdollControlsModifier", 4, EditAreaTableFlag))
    {
        show_hkbModifier(obj, file);

        fullTableSeparator();

        refEdit(obj.getByName("bones"), {"hkbBoneIndexArray"}, file,
                "The bones to be driven by the rigid body ragdoll controller.  If this is empty, all bones will be driven.");
        refEdit(obj.getByName("boneWeights"), {"hkbBoneWeightArray"}, file,
                "A weight for each bone of the ragdoll");

        fullTableSeparator();

        ImGui::TableNextColumn();
        ImGui::BulletText("controlData");
        addTooltip("The control data for an hkbPoweredRagdollModifier.");
        ImGui::TableNextColumn();
        ImGui::TableNextColumn();
        ImGui::BulletText("worldFromModelModeData");
        addTooltip("How to process the world-from-model.");
        ImGui::TableNextColumn();

        {
            auto cdata = obj.getByName("controlData").first_child();
            auto wdata = obj.getByName("worldFromModelModeData").first_child();

            sliderFloatEdit(cdata.getByName("maxForce"), 0.0f, 1000.0f, file,
                            "The maximum force applied to each joint.");
            intScalarEdit(wdata.getByName("poseMatchingBone0"), file, ImGuiDataType_S16,
                          "The root bone used for pose matching.\n\n"
                          "Pose matching uses three bone from the ragdoll skeleton which are assumed to be representative of the pose.\n"
                          "The L-shaped skeleton between these three bones is matched to determine similar poses.");

            sliderFloatEdit(cdata.getByName("tau"), 0.0f, 1.0f, file,
                            "The stiffness of the motors. Larger numbers will result in faster, more abrupt, convergence.");
            intScalarEdit(wdata.getByName("poseMatchingBone1"), file, ImGuiDataType_S16,
                          "A second bone used for pose matching.");

            sliderFloatEdit(cdata.getByName("damping"), 0.0f, 1.0f, file,
                            "The damping of the motors.");
            intScalarEdit(wdata.getByName("poseMatchingBone2"), file, ImGuiDataType_S16,
                          "A third bone used for pose matching.");

            sliderFloatEdit(cdata.getByName("proportionalRecoveryVelocity"), 0.0f, 100.0f, file,
                            "This term is multipled by the error of the motor to compute a velocity for the motor to move it toward zero error.");
            enumEdit(wdata.getByName("mode"), Hkx::e_hkbWorldFromModelModeData_WorldFromModelMode,
                     "How to treat the world-from-model when using the powered ragdoll controller.");

            sliderFloatEdit(cdata.getByName("constantRecoveryVelocity"), 0.0f, 100.0f, file,
                            "This term is added to the velocity of the motor to move it toward zero error.");
        }

        ImGui::EndTable();
    }
}

UICLASS(hkbRigidBodyRagdollControlsModifier)
{
    if (ImGui::BeginTable("hkbRigidBodyRagdollControlsModifier", 4, EditAreaTableFlag))
    {
        show_hkbModifier(obj, file);

        fullTableSeparator();

        refEdit(obj.getByName("bones"), {"hkbBoneIndexArray"}, file,
                "The bones to be driven by the rigid body ragdoll controller.  If this is empty, all bones will be driven.");

        fullTableSeparator();

        ImGui::TableNextColumn();
        ImGui::BulletText("controlData");
        addTooltip("The control data for an hkbRigidBodyRagdollModifier.");
        ImGui::TableNextColumn();

        auto cd = obj.getByName("controlData").first_child();

        sliderFloatEdit(cd.getByName("durationToBlend"), 0.0f, 100.0f, file,
                        "The length of time to blend in the current ragdoll pose when transitioning between powered ragdoll and rigid body ragdoll.\n\n"
                        "When going from powered ragdoll to rigid body ragdoll, there is often a visual discontinuity.\n"
                        "We smooth it out by blending the current ragdoll pose with the pose being driven toward.\n"
                        "See hkbRagdollDriverModifier.");

        auto khcd = cd.getByName("keyFrameHierarchyControlData").first_child();

        sliderFloatEdit(khcd.getByName("hierarchyGain"), 0.0f, 1.0f, file,
                        "This parameter blends the desired target for a bone between model space (0.0) or local space (1.0).\n"
                        "Usually the controller will be much stiffer and more stable when driving to model space.\n"
                        "However local space targeting can look more natural.\n"
                        "It is similar to the deprecated bone controller hierarchyGain parameter");
        sliderFloatEdit(khcd.getByName("velocityGain"), 0.0f, 1.0f, file,
                        "This gain controls the proportion of the difference in velocity that is applied to the bodies.\n"
                        "It dampens the effects of the position control.");
        sliderFloatEdit(khcd.getByName("accelerationGain"), 0.0f, 1.0f, file,
                        "This gain controls the proportion of the difference in acceleration that is applied to the bodies.\n"
                        "It dampens the effects of the velocity control.");
        sliderFloatEdit(khcd.getByName("velocityDamping"), 0.0f, 1.0f, file,
                        "This gain dampens the velocities of the bodies.\n"
                        "The current velocity of the body is scaled by this parameter on every frame before the controller is applied.\n"
                        "It is applied every step and is generally more aggressive than standard linear or angular damping.\n"
                        "A value of 0 means no damping.");
        sliderFloatEdit(khcd.getByName("snapGain"), 0.0f, 1.0f, file,
                        "This gain allows for precise matching between keyframes and the current position.\n"
                        "It works like the m_positionGain: it calculates an optimal\n"
                        "deltaVelocity = (keyFramePosition - currentPosition) / deltaTime\n"
                        "scales it by m_snapGain, clips it against m_snapMaxXXXVelocity and scales it down\n"
                        "if (keyFramePosition - currentPosition) > m_snapMaxXXXXDistance");
        sliderFloatEdit(khcd.getByName("positionGain"), 0.0f, 1.0f, file,
                        "This gain controls the proportion of the difference in position that is applied to the bodies.\n"
                        "It has the most immediate effect. High gain values make the controller very stiff.\n"
                        "Once the controller is too stiff it will tend to overshoot. The velocity gain can help control this.");
        sliderFloatEdit(khcd.getByName("snapMaxLinearVelocity"), 0.0f, 100.0f, file,
                        "See m_snapGain. The linear velocity calculated from the snapGain is clamped to this limit before being applied.");
        sliderFloatEdit(khcd.getByName("positionMaxLinearVelocity"), 0.0f, 100.0f, file,
                        "The position difference is scaled by the inverse delta time to compute a velocity to be applied to the rigid body.\n"
                        "The velocity is first clamped to this limit before it is applied.");
        sliderFloatEdit(khcd.getByName("snapMaxAngularVelocity"), 0.0f, 100.0f, file,
                        "See m_snapGain. The angular velocity calculated from the snapGain is clamped to this limit before being applied.");
        sliderFloatEdit(khcd.getByName("positionMaxAngularVelocity"), 0.0f, 100.0f, file,
                        "The orientation difference is scaled by the inverse delta time to compute an angular velocity to be applied to the rigid body.\n"
                        "The velocity is first clamped to this limit before it is applied.");
        sliderFloatEdit(khcd.getByName("snapMaxLinearDistance"), 0.0f, 10.0f, file,
                        "This sets the max linear distance for the snap gain to work at full strength.\n"
                        "The strength of the controller peaks at this distance.\n"
                        "If the current distance is bigger than m_snapMaxLinearDistance,\n"
                        "the snap velocity will be scaled by sqrt( maxDistane/currentDistance ).");
        sliderFloatEdit(khcd.getByName("snapMaxAngularDistance"), 0.0f, 10.0f, file,
                        "This sets the max angular distance for the snap gain to work at full strength.\n"
                        "The strength of the controller peaks at this distance.\n"
                        "If the current distance is bigger than m_snapMaxAngularDistance,\n"
                        "the snap velocity will be scaled by sqrt( maxDistane/currentDistance ).");

        ImGui::EndTable();
    }
}

UICLASS(hkbRotateCharacterModifier)
{
    if (ImGui::BeginTable("hkbRotateCharacterModifier1", 4, EditAreaTableFlag, {-FLT_MIN, 27 * 3}))
    {
        show_hkbModifier(obj, file);

        fullTableSeparator();

        sliderFloatEdit(obj.getByName("degreesPerSecond"), -720.0f, 720.0f, file,
                        "The speed of rotation.");
        sliderFloatEdit(obj.getByName("speedMultiplier"), -1000.0f, 1000.0f, file,
                        "The speed of rotation multiplier.");
        ImGui::EndTable();
    }
    ImGui::Separator();
    if (ImGui::BeginTable("hkbRotateCharacterModifier2", 2, EditAreaTableFlag))
    {
        quadEdit(obj.getByName("axisOfRotation"), file,
                 "The axis of rotation.");
        ImGui::EndTable();
    }
}

UICLASS(hkbTimerModifier)
{
    if (ImGui::BeginTable("hkbTimerModifier", 4, EditAreaTableFlag))
    {
        show_hkbModifier(obj, file);

        fullTableSeparator();

        sliderFloatEdit(obj.getByName("alarmTimeSeconds"), 0.0f, 100.0f, file,
                        "When the timer alarm goes off.");

        fullTableSeparator();

        ImGui::TableNextColumn();
        ImGui::BulletText("alarmEvent");
        addTooltip("The event id to send when the alarm goes off.");
        ImGui::TableNextRow();
        show_hkbEvent(obj.getByName("alarmEvent").first_child(), file);

        ImGui::EndTable();
    }
}

UICLASS(hkbFootIkControlsModifier)
{
    if (ImGui::BeginTable("hkbFootIkControlsModifier1", 4, EditAreaTableFlag, {-FLT_MIN, 27 * 9}))
    {
        show_hkbModifier(obj, file);

        fullTableSeparator();

        ImGui::TableNextColumn();
        ImGui::BulletText("controlData - gains");
        addTooltip("foot Ik gains.");
        ImGui::TableNextColumn();

        auto gains = obj.getByName("controlData").first_child().getByName("gains").first_child();
        sliderFloatEdit(gains.getByName("onOffGain"), 0.0f, 1.0f, file,
                        "Gain used when transitioning from foot placement on and off. Default value is 0.2.");
        sliderFloatEdit(gains.getByName("groundAscendingGain"), 0.0f, 1.0f, file,
                        "Gain used when the ground height is increasing (going up). Default value is 1.0.");
        sliderFloatEdit(gains.getByName("groundDescendingGain"), 0.0f, 1.0f, file,
                        "Gain used when the ground height is decreasing (going down). Default value is 1.0.");
        sliderFloatEdit(gains.getByName("footPlantedGain"), 0.0f, 1.0f, file,
                        "Gain used when the foot is fully planted.\n"
                        "Depending on the height of the ankle, a value is interpolated between m_footPlantedGain and m_footRaisedGain\n"
                        "and then multiplied by the ascending/descending gain to give the final gain used.\n"
                        "Default value is 1.0.");
        sliderFloatEdit(gains.getByName("worldFromModelFeedbackGain"), 0.0f, 1.0f, file,
                        "Gain used to move the world-from-model transform up or down in order to reduce the amount that the foot IK system needs to move the feet.\n"
                        "Default value is 0.");
        sliderFloatEdit(gains.getByName("footRaisedGain"), 0.0f, 1.0f, file,
                        "Gain used when the foot is fully raised.\n"
                        "Depending on the height of the ankle, a value is interpolated between m_footPlantedGain and m_footRaisedGain\n"
                        "and then multiplied by the ascending/descending gain to give the final gain used.\n"
                        "Default value is 1.0.");
        sliderFloatEdit(gains.getByName("errorUpDownBias"), 0.0f, 1.0f, file,
                        "Use this to bias the error toward moving the character up to avoid compressing the legs (0), or down to avoid stretching the legs (1).\n"
                        "A value below 0.5 biases upward, and above 0.5 biases downward.");
        sliderFloatEdit(gains.getByName("footUnlockGain"), 0.0f, 1.0f, file,
                        "Gain used when the foot becomes unlocked or locked.\n"
                        "When the foot changes between locked and unlocked states, there can be a gap between the previous foot position and the desired position.\n"
                        "This gain is used to smoothly transition to the new foot position.\n"
                        "This gain only affects the horizontal component of the position, since the other gains take care of vertical smoothing.");
        sliderFloatEdit(gains.getByName("alignWorldFromModelGain"), 0.0f, 1.0f, file,
                        "If non-zero, the up vector of the character's worldFromModel of the character is aligned with the floor.\n"
                        "The floor normal is approximated from the raycasts done for each foot (if the character is a biped then two additional rays are cast).\n"
                        "Default value is 0.");
        sliderFloatEdit(gains.getByName("hipOrientationGain"), 0.0f, 1.0f, file,
                        "If non-zero, the hips are rotated to compensate for the rotation of the worldFromModel.\n"
                        "This makes the legs point toward the ground.  The hip rotation is done on the input pose before applying IK.\n"
                        "Default value is 0.");
        floatEdit(gains.getByName("maxKneeAngleDifference"), file);
        floatEdit(gains.getByName("ankleOrientationGain"), file);

        ImGui::EndTable();
    }
    ImGui::Separator();
    if (ImGui::BeginTable("hkbFootIkControlsModifier2", 2, EditAreaTableFlag, {-FLT_MIN, 27 * 2}))
    {
        quadEdit(obj.getByName("errorOutTranslation"), file);
        quadEdit(obj.getByName("alignWithGroundRotation"), file);
        ImGui::EndTable();
    }
    ImGui::Separator();

    static pugi::xml_node obj_cache;
    static pugi::xml_node edit_leg = {};
    if (obj_cache != obj)
        edit_leg = {};
    obj_cache = obj; // workaround to prevent crashing between files, asserting that may only be one menu at the same time

    if (ImGui::BeginTable("hkbFootIkControlsModifier3", 2, ImGuiTableFlags_Resizable))
    {
        ImGui::TableSetupColumn("legs", ImGuiTableColumnFlags_WidthFixed, 200.0F);
        ImGui::TableSetupColumn("editarea", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextColumn();

        objLiveEditList(obj.getByName("legs"), edit_leg, Hkx::g_def_hkbFootIkControlsModifier_Leg, file, "legs",
                        [](auto node) { return fmt::format("{}", getChildIndex(node)); });

        ImGui::TableNextColumn();
        if (edit_leg)
        {
            if (ImGui::BeginTable("hkbFootIkControlsModifier4", 2, EditAreaTableFlag))
            {
                quadEdit(edit_leg.getByName("groundPosition"), file,
                         "The position of the ground below the foot, as computed by the foot IK raycasts.");
                floatEdit(edit_leg.getByName("verticalError"), file,
                          "The distance between the input foot height and the ground.");
                boolEdit(edit_leg.getByName("hitSomething"), file,
                         "Whether or not the ground was hit by the raycast.");
                boolEdit(edit_leg.getByName("isPlantedMS"), file,
                         "Whether or not the foot is planted in model space in the incoming animation.\n"
                         "If the height of the ankle goes below m_footPlantedAnkleHeightMS the foot is considered planted.");

                fullTableSeparator();

                ImGui::TableNextColumn();
                ImGui::BulletText("ungroundedEvent");
                addTooltip("This event is sent if the foot raycast does not find a hit when the foot is supposed to be planted.");
                ImGui::TableNextRow();
                show_hkbEvent(obj.getByName("ungroundedEvent").first_child(), file);

                ImGui::EndTable();
            }
        }
        else
            ImGui::TextDisabled("Not editing any leg");

        ImGui::EndTable();
    }
}

UICLASS(hkbTwistModifier)
{
    if (ImGui::BeginTable("hkbTwistModifier1", 4, EditAreaTableFlag, {-FLT_MIN, 27 * 2}))
    {
        show_hkbModifier(obj, file);
        ImGui::EndTable();
    }
    ImGui::Separator();
    if (ImGui::BeginTable("hkbTwistModifier2", 2, EditAreaTableFlag, {-FLT_MIN, 27 * 1}))
    {
        quadEdit(obj.getByName("axisOfRotation"), file,
                 "The axis of rotation.");
        ImGui::EndTable();
    }
    if (ImGui::BeginTable("hkbTwistModifier3", 4, EditAreaTableFlag))
    {
        sliderFloatEdit(obj.getByName("twistAngle"), -360.0f, 360.0f, file,
                        "The total twist angle to apply to chain of bones");

        fullTableSeparator();

        intScalarEdit(obj.getByName("startBoneIndex"), file, ImGuiDataType_S16,
                      "Index of the first bone in the chain.  This bone must be closer to the root than endBoneIndex.");
        intScalarEdit(obj.getByName("endBoneIndex"), file, ImGuiDataType_S16,
                      "Index of the last bone in the chain.  This bone must be farther from the root than startBoneIndex.");

        fullTableSeparator();

        enumEdit(obj.getByName("setAngleMethod"), Hkx::e_hkbTwistModifier_SetAngleMethod,
                 "Twist angle per bone increased via LINEAR or RAMPED method.");
        enumEdit(obj.getByName("rotationAxisCoordinates"), Hkx::e_hkbTwistModifier_RotationAxisCoordinates,
                 "Whether the m_axisOfRotation is in model space or local space.");
        boolEdit(obj.getByName("isAdditive"), file,
                 "Twist angle per bone is ADDITIVE or NOT");

        ImGui::EndTable();
    }
}

UICLASS(BSDirectAtModifier)
{
    if (ImGui::BeginTable("BSDirectAtModifier1", 4, EditAreaTableFlag, {-FLT_MIN, 27 * 11 + 2 * 5}))
    {
        show_hkbModifier(obj, file);

        fullTableSeparator();

        intScalarEdit(obj.getByName("userInfo"), file, ImGuiDataType_U32);
        boolEdit(obj.getByName("active"), file);

        fullTableSeparator();

        boolEdit(obj.getByName("directAtTarget"), file);
        intScalarEdit(obj.getByName("sourceBoneIndex"), file, ImGuiDataType_S16);
        intScalarEdit(obj.getByName("startBoneIndex"), file, ImGuiDataType_S16,
                      "Index of the first bone in the chain.  This bone must be closer to the root than endBoneIndex.");
        intScalarEdit(obj.getByName("endBoneIndex"), file, ImGuiDataType_S16,
                      "Index of the last bone in the chain.  This bone must be farther from the root than startBoneIndex.");

        fullTableSeparator();

        sliderFloatEdit(obj.getByName("limitHeadingDegrees"), 0.0f, 180.0f, file);
        sliderFloatEdit(obj.getByName("limitPitchDegrees"), 0.0f, 180.0f, file);
        floatEdit(obj.getByName("offsetHeadingDegrees"), file);
        floatEdit(obj.getByName("offsetPitchDegrees"), file);
        sliderFloatEdit(obj.getByName("onGain"), 0.0f, 1.0f, file);
        sliderFloatEdit(obj.getByName("offGain"), 0.0f, 1.0f, file);

        fullTableSeparator();

        boolEdit(obj.getByName("directAtCamera"), file);
        floatEdit(obj.getByName("directAtCameraX"), file);
        floatEdit(obj.getByName("directAtCameraY"), file);
        floatEdit(obj.getByName("directAtCameraZ"), file);

        fullTableSeparator();

        floatEdit(obj.getByName("currentHeadingOffset"), file);
        floatEdit(obj.getByName("currentPitchOffset"), file);

        ImGui::EndTable();
    }
    ImGui::Separator();
    if (ImGui::BeginTable("BSDirectAtModifier2", 2, EditAreaTableFlag))
    {
        quadEdit(obj.getByName("targetLocation"), file);
        ImGui::EndTable();
    }
}

UICLASS(BSEventEveryNEventsModifier)
{
    if (ImGui::BeginTable("BSEventEveryNEventsModifier1", 4, EditAreaTableFlag))
    {
        show_hkbModifier(obj, file);

        fullTableSeparator();

        intScalarEdit(obj.getByName("numberOfEventsBeforeSend"), file, ImGuiDataType_S32);
        intScalarEdit(obj.getByName("minimumNumberOfEventsBeforeSend"), file, ImGuiDataType_S32);
        boolEdit(obj.getByName("randomizeNumberOfEvents"), file);

        fullTableSeparator();

        ImGui::TableNextColumn();
        ImGui::BulletText("eventToCheckFor");
        ImGui::TableNextColumn();
        ImGui::TableNextColumn();
        ImGui::BulletText("eventToSend");
        ImGui::TableNextColumn();

        ImGui::TableNextColumn();
        if (ImGui::BeginTable("BSEventEveryNEventsModifier2", 2, EditAreaTableFlag))
        {
            show_hkbEvent(obj.getByName("eventToCheckFor").first_child(), file);
            ImGui::EndTable();
        }
        ImGui::TableNextColumn();
        ImGui::TableNextColumn();
        if (ImGui::BeginTable("BSEventEveryNEventsModifier3", 2, EditAreaTableFlag))
        {
            show_hkbEvent(obj.getByName("eventToSend").first_child(), file);
            ImGui::EndTable();
        }
        ImGui::TableNextColumn();

        ImGui::EndTable();
    }
}

UICLASS(BSEventOnDeactivateModifier)
{
    if (ImGui::BeginTable("BSEventOnDeactivateModifier1", 4, EditAreaTableFlag, {-FLT_MIN, 27 * 2}))
    {
        show_hkbModifier(obj, file);
        ImGui::EndTable();
    }
    ImGui::Separator();
    ImGui::BulletText("event");
    if (ImGui::BeginTable("BSEventOnDeactivateModifier2", 2, EditAreaTableFlag))
    {
        show_hkbEvent(obj.getByName("event").first_child(), file);
        ImGui::EndTable();
    }
}

UICLASS(BSEventOnFalseToTrueModifier)
{
    if (ImGui::BeginTable("BSEventOnFalseToTrueModifier1", 4, EditAreaTableFlag))
    {
        show_hkbModifier(obj, file);

        fullTableSeparator();

        boolEdit(obj.getByName("bEnableEvent1"), file);
        ImGui::TableNextColumn();
        ImGui::BulletText("EventToSend1");
        ImGui::TableNextColumn();
        boolEdit(obj.getByName("bVariableToTest1"), file);
        ImGui::TableNextColumn();
        if (ImGui::BeginTable("BSEventOnFalseToTrueModifier2", 2, EditAreaTableFlag))
        {
            show_hkbEvent(obj.getByName("EventToSend1").first_child(), file);
            ImGui::EndTable();
        }
        ImGui::TableNextColumn();

        fullTableSeparator();

        boolEdit(obj.getByName("bEnableEvent2"), file);
        ImGui::TableNextColumn();
        ImGui::BulletText("EventToSend2");
        ImGui::TableNextColumn();
        boolEdit(obj.getByName("bVariableToTest2"), file);
        ImGui::TableNextColumn();
        if (ImGui::BeginTable("BSEventOnFalseToTrueModifier3", 2, EditAreaTableFlag))
        {
            show_hkbEvent(obj.getByName("EventToSend2").first_child(), file);
            ImGui::EndTable();
        }
        ImGui::TableNextColumn();

        fullTableSeparator();

        boolEdit(obj.getByName("bEnableEvent3"), file);
        ImGui::TableNextColumn();
        ImGui::BulletText("EventToSend3");
        ImGui::TableNextColumn();
        boolEdit(obj.getByName("bVariableToTest3"), file);
        ImGui::TableNextColumn();
        if (ImGui::BeginTable("BSEventOnFalseToTrueModifier4", 2, EditAreaTableFlag))
        {
            show_hkbEvent(obj.getByName("EventToSend3").first_child(), file);
            ImGui::EndTable();
        }
        ImGui::TableNextColumn();

        ImGui::EndTable();
    }
}

UICLASS(hkbCombineTransformsModifier)
{
    if (ImGui::BeginTable("hkbCombineTransformsModifier1", 4, EditAreaTableFlag, {-FLT_MIN, 27 * 4}))
    {
        show_hkbModifier(obj, file);

        fullTableSeparator();

        boolEdit(obj.getByName("invertLeftTransform"), file,
                 "The left transform is inverted before the transforms are combined.");
        boolEdit(obj.getByName("invertRightTransform"), file,
                 "The right transform is inverted before the transforms are combined.");
        boolEdit(obj.getByName("invertResult"), file,
                 "The resulting transform is inverted after the transforms are combined.");

        ImGui::EndTable();
    }
    ImGui::Separator();
    if (ImGui::BeginTable("hkbCombineTransformsModifier2", 2, EditAreaTableFlag))
    {
        quadEdit(obj.getByName("translationOut"), file,
                 "The translation of the combined transform.");
        quadEdit(obj.getByName("rotationOut"), file,
                 "The orientation of the combined transform.");
        quadEdit(obj.getByName("leftTranslation"), file,
                 "The translation of the left transform to be combined.");
        quadEdit(obj.getByName("leftRotation"), file,
                 "The rotation of the left transform to be combined.");
        quadEdit(obj.getByName("rightTranslation"), file,
                 "The translation of the right transform to be combined.");
        quadEdit(obj.getByName("rightRotation"), file,
                 "The rotation of the right transform to be combined.");

        ImGui::EndTable();
    }
}

UICLASS(hkbComputeDirectionModifier)
{
    if (ImGui::BeginTable("hkbComputeDirectionModifier1", 4, EditAreaTableFlag, {-FLT_MIN, 27 * 2}))
    {
        show_hkbModifier(obj, file);

        ImGui::EndTable();
    }
    ImGui::Separator();
    if (ImGui::BeginTable("hkbComputeDirectionModifier2", 2, EditAreaTableFlag, {-FLT_MIN, 27 * 2}))
    {
        quadEdit(obj.getByName("pointIn"), file,
                 "The point in world coordinates that we want the direction to.");
        quadEdit(obj.getByName("pointOut"), file,
                 "The input point transformed to the character's coordinates.");

        ImGui::EndTable();
    }
    ImGui::Separator();
    if (ImGui::BeginTable("hkbComputeDirectionModifier3", 4, EditAreaTableFlag))
    {
        floatEdit(obj.getByName("groundAngleOut"), file,
                  "The angle in the ground plane to the point in the character's coordinates. This is in the range from -180 to 180.\n"
                  "Zero means directly in front of the character.  Positive angles are to the right of the character.");
        floatEdit(obj.getByName("upAngleOut"), file,
                  "The vertical angle to the target.");
        floatEdit(obj.getByName("verticalOffset"), file,
                  "This is added to m_pointOut in the up direction before computing the output angles.");

        fullTableSeparator();

        boolEdit(obj.getByName("reverseGroundAngle"), file,
                 "Flip the sign of m_groundAngleOut.");
        boolEdit(obj.getByName("reverseUpAngle"), file,
                 "Flip the sign of m_upAngleOut.");
        boolEdit(obj.getByName("projectPoint"), file,
                 "Whether to remove the character's up component from m_pointOut, projecting it onto the ground plane.");
        boolEdit(obj.getByName("normalizePoint"), file,
                 "Whether to normalize m_pointOut to be of length 1.");
        boolEdit(obj.getByName("computeOnlyOnce"), file,
                 "Whether to compute the output only once.");
        boolEdit(obj.getByName("computedOutput"), file);

        ImGui::EndTable();
    }
}

UICLASS(hkbComputeRotationFromAxisAngleModifier)
{
    if (ImGui::BeginTable("hkbComputeRotationFromAxisAngleModifier1", 4, EditAreaTableFlag, {-FLT_MIN, 27 * 2}))
    {
        show_hkbModifier(obj, file);

        ImGui::EndTable();
    }
    ImGui::Separator();
    if (ImGui::BeginTable("hkbComputeRotationFromAxisAngleModifier2", 2, EditAreaTableFlag))
    {
        sliderFloatEdit(obj.getByName("angleDegrees"), -360.0f, 360.0f, file,
                        "The number of degrees to rotate.");
        quadEdit(obj.getByName("axis"), file,
                 "The axis of rotation.");
        quadEdit(obj.getByName("rotationOut"), file,
                 "The output rotation.");

        ImGui::EndTable();
    }
}

UICLASS(hkbComputeRotationToTargetModifier)
{
    if (ImGui::BeginTable("hkbComputeRotationToTargetModifier1", 4, EditAreaTableFlag, {-FLT_MIN, 27 * 2}))
    {
        show_hkbModifier(obj, file);

        ImGui::EndTable();
    }
    ImGui::Separator();
    if (ImGui::BeginTable("hkbComputeRotationToTargetModifier2", 2, EditAreaTableFlag))
    {
        quadEdit(obj.getByName("targetPosition"), file,
                 "The position of the facee/target.");
        quadEdit(obj.getByName("currentPosition"), file,
                 "The current position of the facer.");
        quadEdit(obj.getByName("currentRotation"), file,
                 "The current rotation of the facer.");
        quadEdit(obj.getByName("localAxisOfRotation"), file,
                 "The axis of rotation in the local space of the facer.");
        quadEdit(obj.getByName("localFacingDirection"), file,
                 "The facing direction in the local space of the facer.");
        quadEdit(obj.getByName("rotationOut"), file,
                 "The modified rotation.");

        boolEdit(obj.getByName("resultIsDelta"), file,
                 "The resulting transform is a delta from the initial.");

        ImGui::EndTable();
    }
}

UICLASS(hkbDampingModifier)
{
    if (ImGui::BeginTable("hkbComputeRotationToTargetModifier1", 4, EditAreaTableFlag, {-FLT_MIN, 27 * 7 + 2 * 3}))
    {
        show_hkbModifier(obj, file);

        fullTableSeparator();

        sliderFloatEdit(obj.getByName("kP"), -3.0f, 3.0f, file,
                        "The coefficient for the proportional component of the damping system.");
        sliderFloatEdit(obj.getByName("kI"), -3.0f, 3.0f, file,
                        "The coefficient for the integral component of the damping system.");
        sliderFloatEdit(obj.getByName("kD"), -3.0f, 3.0f, file,
                        "The coefficient for the derivative component of the damping system.");

        fullTableSeparator();

        boolEdit(obj.getByName("enableScalarDamping"), file,
                 "Enable/disable scalar damping.");
        boolEdit(obj.getByName("enableVectorDamping"), file,
                 "Enable/disable Vector4 damping.");

        fullTableSeparator();

        if (!obj.getByName("enableScalarDamping").text().as_bool())
            ImGui::BeginDisabled();
        sliderFloatEdit(obj.getByName("rawValue"), -10000.0f, 10000.0f, file,
                        "The value that is being damped.");
        sliderFloatEdit(obj.getByName("dampedValue"), -10000.0f, 10000.0f, file,
                        "The resulting damped value.");
        sliderFloatEdit(obj.getByName("previousError"), -100.0f, 100.0f, file,
                        "The previous error.");
        sliderFloatEdit(obj.getByName("errorSum"), -100.0f, 100.0f, file,
                        "The sum of the errors so far.");
        if (!obj.getByName("enableScalarDamping").text().as_bool())
            ImGui::EndDisabled();

        ImGui::EndTable();
    }
    ImGui::Separator();
    if (ImGui::BeginTable("hkbComputeRotationToTargetModifier2", 2, EditAreaTableFlag))
    {
        if (!obj.getByName("enableVectorDamping").text().as_bool())
            ImGui::BeginDisabled();
        quadEdit(obj.getByName("rawVector"), file,
                 "The vector being damped.");
        quadEdit(obj.getByName("dampedVector"), file,
                 "The resulting damped vector.");
        quadEdit(obj.getByName("vecPreviousError"), file,
                 "The previous error for the damped vector.");
        quadEdit(obj.getByName("vecErrorSum"), file,
                 "The sum of errors so far for the damped vector.");
        if (!obj.getByName("enableVectorDamping").text().as_bool())
            ImGui::EndDisabled();

        ImGui::EndTable();
    }
}

UICLASS(hkbDelayedModifier)
{
    if (ImGui::BeginTable("hkbDelayedModifier", 4, EditAreaTableFlag))
    {
        show_hkbModifier(obj, file);

        fullTableSeparator();

        refEdit(obj.getByName("modifier"), {Hkx::g_class_modifiers.begin(), Hkx::g_class_modifiers.end()}, file);

        fullTableSeparator();

        sliderFloatEdit(obj.getByName("delaySeconds"), 0.0f, 1000.0f, file,
                        "The number of seconds to delay before applying the modifier.");
        sliderFloatEdit(obj.getByName("durationSeconds"), 0.0f, 1000.0f, file,
                        "The number of seconds to apply the modifier after the delay.  Set this to zero to apply the modifier indefinitely.");

        ImGui::EndTable();
    }
}

UICLASS(hkbDetectCloseToGroundModifier)
{
    if (ImGui::BeginTable("hkbDetectCloseToGroundModifier1", 4, EditAreaTableFlag))
    {
        show_hkbModifier(obj, file);

        fullTableSeparator();

        intScalarEdit(obj.getByName("boneIndex"), file, ImGuiDataType_S16,
                      "The bone used to cast ray for determining the ground height.");
        intScalarEdit(obj.getByName("animBoneIndex"), file, ImGuiDataType_S16,
                      "The bone used to cast ray for determining the ground height.");

        fullTableSeparator();

        sliderFloatEdit(obj.getByName("closeToGroundHeight"), 0.0f, 100.0f, file,
                        "If the specified bone is below this height it is considered to be close to the ground.");
        sliderFloatEdit(obj.getByName("raycastDistanceDown"), 0.0f, 100.0f, file,
                        "When ray casting from the bone towards the ground, the (positive) distance, from the bone and in the down direction, where the ray ends.\n"
                        "Default is 0.8(m), you may want to change this according to the scale of your character.");
        intScalarEdit(obj.getByName("collisionFilterInfo"), file, ImGuiDataType_U32,
                      "The collision filter info to use when raycasting the ground.");

        fullTableSeparator();

        ImGui::TableNextColumn();
        ImGui::BulletText("closeToGroundEvent");
        addTooltip("Event to send when the bone is below the m_closeToGroundHeight.");
        ImGui::TableNextRow();
        show_hkbEvent(obj.getByName("closeToGroundEvent").first_child(), file);

        ImGui::EndTable();
    }
}

UICLASS(hkbExtractRagdollPoseModifier)
{
    if (ImGui::BeginTable("hkbExtractRagdollPoseModifier", 4, EditAreaTableFlag))
    {
        show_hkbModifier(obj, file);

        fullTableSeparator();

        intScalarEdit(obj.getByName("poseMatchingBone0"), file, ImGuiDataType_S16,
                      "A bone to use for pose matching when computing the world-from-model.");
        intScalarEdit(obj.getByName("poseMatchingBone1"), file, ImGuiDataType_S16,
                      "A bone to use for pose matching when computing the world-from-model.");
        intScalarEdit(obj.getByName("poseMatchingBone2"), file, ImGuiDataType_S16,
                      "A bone to use for pose matching when computing the world-from-model.");
        boolEdit(obj.getByName("enableComputeWorldFromModel"), file,
                 "Whether to enable the computation of the world-from-model from the ragdoll pose.");

        ImGui::EndTable();
    }
}

UICLASS(hkbGetWorldFromModelModifier)
{
    if (ImGui::BeginTable("hkbGetWorldFromModelModifier1", 4, EditAreaTableFlag, {-FLT_MIN, 27 * 2}))
    {
        show_hkbModifier(obj, file);
        ImGui::EndTable();
    }
    ImGui::Separator();
    if (ImGui::BeginTable("hkbGetWorldFromModelModifier2", 2, EditAreaTableFlag))
    {
        quadEdit(obj.getByName("translationOut"), file,
                 "The current position of world from model.");
        quadEdit(obj.getByName("rotationOut"), file,
                 "The current rotation of world from model.");
        ImGui::EndTable();
    }
}

UICLASS(hkbLookAtModifier)
{
    if (ImGui::BeginTable("hkbLookAtModifier1", 4, EditAreaTableFlag, {-FLT_MIN, 27 * 2}))
    {
        show_hkbModifier(obj, file);
        ImGui::EndTable();
    }
    ImGui::Separator();
    if (ImGui::BeginTable("hkbLookAtModifier2", 2, EditAreaTableFlag, {-FLT_MIN, 27 * 5}))
    {
        quadEdit(obj.getByName("targetWS"), file,
                 "The target position in world coordinates.");
        quadEdit(obj.getByName("headForwardLS"), file,
                 "The forward direction in the coordinates of the head. Default is (0,1,0).");
        quadEdit(obj.getByName("neckForwardLS"), file,
                 "The forward direction in the coordinates of the neck. Default is (0,1,0).");
        quadEdit(obj.getByName("neckRightLS"), file,
                 "The right direction in the coordinates of the neck. Default is (1,0,0).");
        quadEdit(obj.getByName("eyePositionHS"), file,
                 "The \"Eye\" position in the coordinates of the head. Default is (0,0,0).");
        ImGui::EndTable();
    }
    ImGui::Separator();
    if (ImGui::BeginTable("hkbLookAtModifier3", 4, EditAreaTableFlag))
    {
        sliderFloatEdit(obj.getByName("newTargetGain"), 0.0f, 1.0f, file,
                        "Gain used when a new target is found. This gain is used to smoothly transition from the old target to the new target.\n"
                        "Default value is 0.2.");
        sliderFloatEdit(obj.getByName("limitAngleDegrees"), 0.0f, 180.0f, file,
                        "Angle of the lookAt limiting cone, in degrees. Must be in range [ 0, 180 ]");
        sliderFloatEdit(obj.getByName("onGain"), 0.0f, 1.0f, file,
                        "Gain used to smooth out the motion when the character starts looking at a target. Default value is 0.05.");
        floatEdit(obj.getByName("limitAngleLeft"), file,
                  "The maximum angle to look left");
        sliderFloatEdit(obj.getByName("offGain"), 0.0f, 1.0f, file,
                        "Gain used to smooth out the motion when the character stops looking at a target. Default value is 0.05.");
        floatEdit(obj.getByName("limitAngleRight"), file,
                  "The maximum angle to look right");
        intScalarEdit(obj.getByName("headIndex"), file, ImGuiDataType_S16,
                      "Index of the head bone. If this is not -1, it overrides the character boneInfo.");
        floatEdit(obj.getByName("limitAngleUp"), file,
                  "The maximum angle to look up");
        intScalarEdit(obj.getByName("neckIndex"), file, ImGuiDataType_S16,
                      "Index of the neck bone. If this is not -1, it overrides the character boneInfo.");
        floatEdit(obj.getByName("limitAngleDown"), file,
                  "The maximum angle to look down");

        fullTableSeparator();

        boolEdit(obj.getByName("isOn"), file,
                 "Whether or not the IK is on (if off it will fade out).");
        boolEdit(obj.getByName("individualLimitsOn"), file,
                 "Whether a single limit (m_limitAngleDegrees) or individual limits are used");
        boolEdit(obj.getByName("isTargetInsideLimitCone"), file,
                 "Whether the target point was found to be inside the target cone.\n"
                 "This also becomes false if IK is not run at all because it is turned off or the target is behind the character.");

        ImGui::EndTable();
    }
}

UICLASS(hkbMirrorModifier)
{
    if (ImGui::BeginTable("hkbMirrorModifier", 4, EditAreaTableFlag))
    {
        show_hkbModifier(obj, file);

        fullTableSeparator();

        boolEdit(obj.getByName("isAdditive"), file,
                 "Is the input pose an additive pose");

        ImGui::EndTable();
    }
}

UICLASS(hkbMoveCharacterModifier)
{
    if (ImGui::BeginTable("hkbMoveCharacterModifier1", 4, EditAreaTableFlag, {-FLT_MIN, 27 * 2}))
    {
        show_hkbModifier(obj, file);
        ImGui::EndTable();
    }
    ImGui::Separator();
    if (ImGui::BeginTable("hkbMoveCharacterModifier2", 2, EditAreaTableFlag))
    {
        quadEdit(obj.getByName("offsetPerSecondMS"), file,
                 "The offset to move the character per second, in model space.");
        ImGui::EndTable();
    }
}

UICLASS(hkbTransformVectorModifier)
{
    if (ImGui::BeginTable("hkbTransformVectorModifier1", 4, EditAreaTableFlag, {-FLT_MIN, 27 * 4}))
    {
        show_hkbModifier(obj, file);

        fullTableSeparator();

        boolEdit(obj.getByName("rotateOnly"), file,
                 "Just rotate the vector, don't transform it as a point.");
        boolEdit(obj.getByName("inverse"), file,
                 "Apply the inverse of the transformation.");
        boolEdit(obj.getByName("computeOnActivate"), file,
                 "Do the transform when the modifier is activated.");
        boolEdit(obj.getByName("computeOnModify"), file,
                 "Do the transform every time that modify is called.");

        ImGui::EndTable();
    }
    ImGui::Separator();
    if (ImGui::BeginTable("hkbTransformVectorModifier2", 2, EditAreaTableFlag))
    {
        quadEdit(obj.getByName("rotation"), file,
                 "The rotational part of the transform to apply.");
        quadEdit(obj.getByName("translation"), file,
                 "The translational part of the transform to apply.");
        quadEdit(obj.getByName("vectorIn"), file,
                 "The vector to be transformed.");
        quadEdit(obj.getByName("vectorOut"), file,
                 "The transformed vector.");
        ImGui::EndTable();
    }
}

UICLASS(BSComputeAddBoneAnimModifier)
{
    if (ImGui::BeginTable("BSComputeAddBoneAnimModifier1", 4, EditAreaTableFlag, {-FLT_MIN, 27 * 3}))
    {
        show_hkbModifier(obj, file);

        fullTableSeparator();

        intScalarEdit(obj.getByName("boneIndex"), file, ImGuiDataType_S16);

        ImGui::EndTable();
    }
    ImGui::Separator();
    if (ImGui::BeginTable("BSComputeAddBoneAnimModifier2", 2, EditAreaTableFlag))
    {
        quadEdit(obj.getByName("translationLSOut"), file);
        quadEdit(obj.getByName("rotationLSOut"), file);
        quadEdit(obj.getByName("scaleLSOut"), file);
        ImGui::EndTable();
    }
}

UICLASS(BSDecomposeVectorModifier)
{
    if (ImGui::BeginTable("BSDecomposeVectorModifier1", 4, EditAreaTableFlag, {-FLT_MIN, 27 * 2}))
    {
        show_hkbModifier(obj, file);
        ImGui::EndTable();
    }
    ImGui::Separator();
    if (ImGui::BeginTable("BSDecomposeVectorModifier2", 2, EditAreaTableFlag, {-FLT_MIN, 27}))
    {
        quadEdit(obj.getByName("vector"), file);
        ImGui::EndTable();
    }
    ImGui::Separator();
    if (ImGui::BeginTable("BSDecomposeVectorModifier3", 4, EditAreaTableFlag))
    {
        floatEdit(obj.getByName("x"), file);
        floatEdit(obj.getByName("y"), file);
        floatEdit(obj.getByName("z"), file);
        floatEdit(obj.getByName("w"), file);
        ImGui::EndTable();
    }
}

UICLASS(BSDistTriggerModifier)
{
    if (ImGui::BeginTable("BSDistTriggerModifier1", 4, EditAreaTableFlag, {-FLT_MIN, 27 * 2}))
    {
        show_hkbModifier(obj, file);
        ImGui::EndTable();
    }
    ImGui::Separator();
    if (ImGui::BeginTable("BSDistTriggerModifier2", 2, EditAreaTableFlag, {-FLT_MIN, 27}))
    {
        quadEdit(obj.getByName("targetPosition"), file);
        ImGui::EndTable();
    }
    ImGui::Separator();
    if (ImGui::BeginTable("BSDistTriggerModifier3", 4, EditAreaTableFlag))
    {
        floatEdit(obj.getByName("distance"), file);
        floatEdit(obj.getByName("distanceTrigger"), file);

        fullTableSeparator();

        ImGui::TableNextColumn();
        ImGui::BulletText("triggerEvent");
        ImGui::TableNextRow();
        show_hkbEvent(obj.getByName("triggerEvent").first_child(), file);

        ImGui::EndTable();
    }
}

UICLASS(BSGetTimeStepModifier)
{
    if (ImGui::BeginTable("BSGetTimeStepModifier", 4, EditAreaTableFlag))
    {
        show_hkbModifier(obj, file);

        fullTableSeparator();

        floatEdit(obj.getByName("timeStep"), file);

        ImGui::EndTable();
    }
}

UICLASS(BSInterpValueModifier)
{
    if (ImGui::BeginTable("BSInterpValueModifier", 4, EditAreaTableFlag))
    {
        show_hkbModifier(obj, file);

        fullTableSeparator();

        floatEdit(obj.getByName("source"), file);
        floatEdit(obj.getByName("target"), file);
        floatEdit(obj.getByName("result"), file);
        floatEdit(obj.getByName("gain"), file);

        ImGui::EndTable();
    }
}

UICLASS(BSLimbIKModifier)
{
    if (ImGui::BeginTable("BSLimbIKModifier", 4, EditAreaTableFlag))
    {
        show_hkbModifier(obj, file);

        fullTableSeparator();

        intScalarEdit(obj.getByName("startBoneIndex"), file, ImGuiDataType_S16);
        intScalarEdit(obj.getByName("endBoneIndex"), file, ImGuiDataType_S16);

        fullTableSeparator();

        floatEdit(obj.getByName("limitAngleDegrees"), file);
        floatEdit(obj.getByName("gain"), file);
        floatEdit(obj.getByName("boneRadius"), file);
        floatEdit(obj.getByName("castOffset"), file);

        ImGui::EndTable();
    }
}

UICLASS(BSIsActiveModifier)
{
    if (ImGui::BeginTable("BSIsActiveModifier", 4, EditAreaTableFlag))
    {
        show_hkbModifier(obj, file);

        fullTableSeparator();

        boolEdit(obj.getByName("bIsActive0"), file);
        boolEdit(obj.getByName("bInvertActive0"), file);
        boolEdit(obj.getByName("bIsActive1"), file);
        boolEdit(obj.getByName("bInvertActive1"), file);
        boolEdit(obj.getByName("bIsActive2"), file);
        boolEdit(obj.getByName("bInvertActive2"), file);
        boolEdit(obj.getByName("bIsActive3"), file);
        boolEdit(obj.getByName("bInvertActive3"), file);
        boolEdit(obj.getByName("bIsActive4"), file);
        boolEdit(obj.getByName("bInvertActive4"), file);

        ImGui::EndTable();
    }
}

UICLASS(BSLookAtModifier)
{
    if (ImGui::BeginTable("BSLookAtModifier1", 4, EditAreaTableFlag, {-FLT_MIN, 27 * 10 + 2 * 4}))
    {
        show_hkbModifier(obj, file);

        fullTableSeparator();

        boolEdit(obj.getByName("lookAtTarget"), file);
        boolEdit(obj.getByName("useBoneGains"), file);

        fullTableSeparator();

        floatEdit(obj.getByName("onGain"), file);
        floatEdit(obj.getByName("offGain"), file);

        fullTableSeparator();

        floatEdit(obj.getByName("limitAngleDegrees"), file);
        floatEdit(obj.getByName("distanceTriglimitAngleThresholdDegreesger"), file);
        boolEdit(obj.getByName("targetOutsideLimits"), file);
        boolEdit(obj.getByName("continueLookOutsideOfLimit"), file);

        fullTableSeparator();

        ImGui::TableNextColumn();
        ImGui::BulletText("targetOutOfLimitEvent");
        ImGui::TableNextColumn();
        show_hkbEvent(obj.getByName("targetOutOfLimitEvent").first_child(), file);

        fullTableSeparator();

        boolEdit(obj.getByName("lookAtCamera"), file);
        floatEdit(obj.getByName("lookAtCameraX"), file);
        floatEdit(obj.getByName("lookAtCameraY"), file);
        floatEdit(obj.getByName("lookAtCameraZ"), file);

        ImGui::EndTable();
    }
    ImGui::Separator();
    if (ImGui::BeginTable("BSLookAtModifier2", 2, EditAreaTableFlag, {-FLT_MIN, 27}))
    {
        quadEdit(obj.getByName("targetLocation"), file);
        ImGui::EndTable();
    }
    ImGui::Separator();
    if (ImGui::BeginTable("BSLookAtModifier3", 2, ImGuiTableFlags_SizingStretchSame))
    {
        ImGui::TableNextColumn();
        BSLookAtModifierBoneArray(obj.getByName("bones"), file);
        ImGui::TableNextColumn();
        BSLookAtModifierBoneArray(obj.getByName("eyeBones"), file);

        ImGui::EndTable();
    }
}

UICLASS(BSModifyOnceModifier)
{
    if (ImGui::BeginTable("BSModifyOnceModifier", 4, EditAreaTableFlag))
    {
        show_hkbModifier(obj, file);

        fullTableSeparator();

        refEdit(obj.getByName("pOnActivateModifier"), {Hkx::g_class_modifiers.begin(), Hkx::g_class_modifiers.end()}, file);
        refEdit(obj.getByName("pOnDeactivateModifier"), {Hkx::g_class_modifiers.begin(), Hkx::g_class_modifiers.end()}, file);

        ImGui::EndTable();
    }
}

UICLASS(BSPassByTargetTriggerModifier)
{
    if (ImGui::BeginTable("BSPassByTargetTriggerModifier1", 4, EditAreaTableFlag, {-FLT_MIN, 27 * 2}))
    {
        show_hkbModifier(obj, file);
        ImGui::EndTable();
    }
    ImGui::Separator();
    if (ImGui::BeginTable("BSPassByTargetTriggerModifier2", 2, EditAreaTableFlag, {-FLT_MIN, 27 * 2}))
    {
        quadEdit(obj.getByName("targetPosition"), file);
        quadEdit(obj.getByName("movementDirection"), file);
        ImGui::EndTable();
    }
    ImGui::Separator();
    if (ImGui::BeginTable("BSPassByTargetTriggerModifier3", 4, EditAreaTableFlag))
    {
        floatEdit(obj.getByName("radius"), file);

        fullTableSeparator();

        ImGui::TableNextColumn();
        ImGui::BulletText("triggerEvent");
        ImGui::TableNextRow();
        show_hkbEvent(obj.getByName("triggerEvent").first_child(), file);

        ImGui::EndTable();
    }
}

UICLASS(BSRagdollContactListenerModifier)
{
    if (ImGui::BeginTable("BSRagdollContactListenerModifier1", 4, EditAreaTableFlag))
    {
        show_hkbModifier(obj, file);

        fullTableSeparator();

        refEdit(obj.getByName("bones"), {"hkbBoneIndexArray"}, file);

        fullTableSeparator();

        ImGui::TableNextColumn();
        ImGui::BulletText("contactEvent");
        ImGui::TableNextRow();
        show_hkbEvent(obj.getByName("contactEvent").first_child(), file);

        ImGui::EndTable();
    }
}

UICLASS(BSTimerModifier)
{
    if (ImGui::BeginTable("BSTimerModifier", 4, EditAreaTableFlag))
    {
        show_hkbModifier(obj, file);

        fullTableSeparator();

        floatEdit(obj.getByName("alarmTimeSeconds"), file);
        boolEdit(obj.getByName("resetAlarm"), file);

        fullTableSeparator();

        ImGui::TableNextColumn();
        ImGui::BulletText("alarmEvent");
        ImGui::TableNextRow();
        show_hkbEvent(obj.getByName("alarmEvent").first_child(), file);

        ImGui::EndTable();
    }
}

UICLASS(BSTweenerModifier)
{
    if (ImGui::BeginTable("BSTweenerModifier1", 4, EditAreaTableFlag, {-FLT_MIN, 27 * 4}))
    {
        show_hkbModifier(obj, file);

        fullTableSeparator();

        boolEdit(obj.getByName("targetPosition"), file);
        boolEdit(obj.getByName("tweenRotation"), file);
        boolEdit(obj.getByName("useTweenDuration"), file);
        floatEdit(obj.getByName("tweenDuration"), file);

        ImGui::EndTable();
    }
    ImGui::Separator();
    if (ImGui::BeginTable("BSTweenerModifier2", 2, EditAreaTableFlag))
    {
        quadEdit(obj.getByName("targetPosition"), file);
        quadEdit(obj.getByName("targetRotation"), file);
        ImGui::EndTable();
    }
}

UICLASS(BSSpeedSamplerModifier)
{
    if (ImGui::BeginTable("BSSpeedSamplerModifier", 4, EditAreaTableFlag))
    {
        show_hkbModifier(obj, file);

        fullTableSeparator();

        intScalarEdit(obj.getByName("state"), file);
        floatEdit(obj.getByName("direction"), file);
        floatEdit(obj.getByName("goalSpeed"), file);
        floatEdit(obj.getByName("speedOut"), file);

        ImGui::EndTable();
    }
}

#define UIMAPITEM(hkclass)       \
    {                            \
#        hkclass, show_##hkclass \
    }

const robin_hood::unordered_map<std::string_view, std::function<void(pugi::xml_node, Hkx::HkxFile&)>> g_class_ui_map =
    {UIMAPITEM(hkbBehaviorGraph),
     UIMAPITEM(hkbVariableBindingSet),
     UIMAPITEM(hkbStateMachine),
     UIMAPITEM(hkbStateMachineStateInfo),
     UIMAPITEM(hkbStateMachineTransitionInfoArray),
     UIMAPITEM(hkbBlendingTransitionEffect),
     UIMAPITEM(hkbBlenderGenerator),
     UIMAPITEM(hkbBlenderGeneratorChild),
     UIMAPITEM(BSBoneSwitchGenerator),
     UIMAPITEM(BSBoneSwitchGeneratorBoneData),
     UIMAPITEM(hkbBoneWeightArray),
     UIMAPITEM(hkbBoneIndexArray),
     UIMAPITEM(hkbClipGenerator),
     UIMAPITEM(hkbClipTriggerArray),
     UIMAPITEM(BSSynchronizedClipGenerator),
     UIMAPITEM(hkbManualSelectorGenerator),
     UIMAPITEM(hkbModifierGenerator),
     UIMAPITEM(hkbBehaviorReferenceGenerator),
     UIMAPITEM(hkbPoseMatchingGenerator),
     UIMAPITEM(BSCyclicBlendTransitionGenerator),
     UIMAPITEM(BSiStateTaggingGenerator),
     UIMAPITEM(BSOffsetAnimationGenerator),
     UIMAPITEM(hkbModifierList),
     UIMAPITEM(hkbEvaluateExpressionModifier),
     UIMAPITEM(hkbExpressionDataArray),
     UIMAPITEM(hkbEventDrivenModifier),
     UIMAPITEM(hkbEventsFromRangeModifier),
     UIMAPITEM(hkbEventRangeDataArray),
     UIMAPITEM(hkbGetUpModifier),
     UIMAPITEM(hkbKeyframeBonesModifier),
     UIMAPITEM(hkbPoweredRagdollControlsModifier),
     UIMAPITEM(hkbRigidBodyRagdollControlsModifier),
     UIMAPITEM(hkbRotateCharacterModifier),
     UIMAPITEM(hkbTimerModifier),
     UIMAPITEM(hkbFootIkControlsModifier),
     UIMAPITEM(hkbTwistModifier),
     UIMAPITEM(BSDirectAtModifier),
     UIMAPITEM(BSEventEveryNEventsModifier),
     UIMAPITEM(BSEventOnDeactivateModifier),
     UIMAPITEM(BSEventOnFalseToTrueModifier),
     UIMAPITEM(hkbCombineTransformsModifier),
     UIMAPITEM(hkbComputeDirectionModifier),
     UIMAPITEM(hkbComputeRotationFromAxisAngleModifier),
     UIMAPITEM(hkbComputeRotationToTargetModifier),
     UIMAPITEM(hkbDampingModifier),
     UIMAPITEM(hkbDelayedModifier),
     UIMAPITEM(hkbDetectCloseToGroundModifier),
     UIMAPITEM(hkbExtractRagdollPoseModifier),
     UIMAPITEM(hkbGetWorldFromModelModifier),
     UIMAPITEM(hkbLookAtModifier),
     UIMAPITEM(hkbMirrorModifier),
     UIMAPITEM(hkbMoveCharacterModifier),
     UIMAPITEM(hkbTransformVectorModifier),
     UIMAPITEM(BSComputeAddBoneAnimModifier),
     UIMAPITEM(BSDecomposeVectorModifier),
     UIMAPITEM(BSDistTriggerModifier),
     UIMAPITEM(BSGetTimeStepModifier),
     UIMAPITEM(BSInterpValueModifier),
     UIMAPITEM(BSIsActiveModifier),
     UIMAPITEM(BSLimbIKModifier),
     UIMAPITEM(BSLookAtModifier),
     UIMAPITEM(BSModifyOnceModifier),
     UIMAPITEM(BSPassByTargetTriggerModifier),
     UIMAPITEM(BSRagdollContactListenerModifier),
     UIMAPITEM(BSTimerModifier),
     UIMAPITEM(BSTweenerModifier),
     UIMAPITEM(BSSpeedSamplerModifier)};

void showEditUi(pugi::xml_node hkobject, Hkx::HkxFile& file)
{
    auto hkclass = hkobject.attribute("class").as_string();
    if (g_class_ui_map.contains(hkclass))
        g_class_ui_map.at(hkclass)(hkobject, file);
    else
        ImGui::TextDisabled("The class %s is currently not supported.", hkclass);
}

} // namespace Ui
} // namespace Haviour