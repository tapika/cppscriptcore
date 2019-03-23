#include "pch.h"
#include "VCConfiguration.h"
#include "Project.h"

using namespace pugi;
using namespace std;

//
//  Reconstructs xml path from our class structure to xml nodes.
//
void ReflectCopy(ReflectPath& path, xml_node toNode)
{
    xml_node current = toNode;

    for (size_t i = path.steps.size(); i-- > 0; )
    {
        ReflectPathStep& step = path.steps[i];

        // Field map, which specified in which order fields should be. 0,1 ... and so on.
        auto&& mapFields = step.instance->mapFieldToIndex;

        if (step.instance->propertyName.length() == 0 && step.instance->GetParent() != nullptr)
            mapFields = step.instance->GetParent()->mapFieldToIndex;

        wstring name = as_wide(step.propertyName);
        xml_node next = current.child(name.c_str());
        if (next.empty())
        {
            int newNodeFieldIndex = mapFields[step.propertyName];
            xml_node node = current.first_child();
            xml_node insertBefore;

            for (; !node.empty(); node = node.next_sibling())
            {
                int currentNodeFieldIndex = mapFields[as_utf8(node.name()).c_str()];

                if (newNodeFieldIndex > currentNodeFieldIndex)
                    continue;

                insertBefore = node;
                break;
            }

            if (insertBefore.empty())
                next = current.append_child(name.c_str());
            else
                next = current.insert_child_before(name.c_str(), insertBefore);
        }

        current = next;
    }

    FieldInfo* fi = path.steps[0].typeInfo->GetField(path.steps[0].propertyName);
    CStringW value = fi->fieldType->ToString((char*)path.steps[0].instance->ReflectGetInstance() + fi->offset);
    current.text().set(value);
}

void VCConfiguration::OnAfterSetProperty(ReflectPath& path)
{
    xml_node current;
    ReflectPathStep& lastStep = path.steps.back();

    if (lastStep.typeInfo->name == "GeneralConf")
    {
        if (lastStep.typeInfo->GetField(lastStep.propertyName) - lastStep.typeInfo->GetField("ConfigurationType") >= 0)
        {
            current = pgConfigurationNode;
        }
        else
        {
            if (pgNode.empty())
                pgNode = project->selectProjectNodes(L"PropertyGroup", L"", configurationName.c_str(), platform.c_str());

            current = pgNode;
        }
    }
    else
    {
        if (idgConfNode.empty())
            idgConfNode = project->selectProjectNodes(L"ItemDefinitionGroup", L"", configurationName.c_str(), platform.c_str());

        current = idgConfNode;
    }

    ReflectCopy(path, current);
}

