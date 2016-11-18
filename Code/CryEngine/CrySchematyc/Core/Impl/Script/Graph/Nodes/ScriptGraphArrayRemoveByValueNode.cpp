// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Script/Graph/Nodes/ScriptGraphArrayRemoveByValueNode.h"

#include <Schematyc/Compiler/IGraphNodeCompiler.h>
#include <Schematyc/Env/Elements/IEnvDataType.h>
#include <Schematyc/Utils/Any.h>
#include <Schematyc/Utils/AnyArray.h>
#include <Schematyc/Utils/StackString.h>

#include "Script/ScriptView.h"
#include "Script/Graph/ScriptGraphNode.h"
#include "Script/Graph/ScriptGraphNodeFactory.h"
#include "SerializationUtils/SerializationContext.h"

namespace Schematyc
{
CScriptGraphArrayRemoveByValueNode::CScriptGraphArrayRemoveByValueNode(const SElementId& reference)
	: m_defaultValue(reference)
{}

SGUID CScriptGraphArrayRemoveByValueNode::GetTypeGUID() const
{
	return ms_typeGUID;
}

void CScriptGraphArrayRemoveByValueNode::CreateLayout(CScriptGraphNodeLayout& layout)
{
	layout.SetName("Array::RemoveByValue");
	layout.SetColor(EScriptGraphColor::Purple);

	layout.AddInput("In", SGUID(), { EScriptGraphPortFlags::Flow, EScriptGraphPortFlags::MultiLink });
	layout.AddOutput("Default", SGUID(), { EScriptGraphPortFlags::Flow, EScriptGraphPortFlags::SpacerBelow });

	if (!m_defaultValue.IsEmpty())
	{
		const SGUID typeGUID = m_defaultValue.GetTypeId().guid;
		layout.AddInput("Array", typeGUID, { EScriptGraphPortFlags::Data, EScriptGraphPortFlags::Array });
		layout.AddInputWithData(m_defaultValue.GetTypeName(), typeGUID, { EScriptGraphPortFlags::Data, EScriptGraphPortFlags::Persistent, EScriptGraphPortFlags::Editable }, *m_defaultValue.GetValue());
	}
}

void CScriptGraphArrayRemoveByValueNode::Compile(SCompilerContext& context, IGraphNodeCompiler& compiler) const
{
	compiler.BindCallback(&Execute);
}

void CScriptGraphArrayRemoveByValueNode::LoadDependencies(Serialization::IArchive& archive, const ISerializationContext& context)
{
	m_defaultValue.SerializeTypeId(archive);
}

void CScriptGraphArrayRemoveByValueNode::Save(Serialization::IArchive& archive, const ISerializationContext& context)
{
	m_defaultValue.SerializeTypeId(archive);
}

void CScriptGraphArrayRemoveByValueNode::Edit(Serialization::IArchive& archive, const ISerializationContext& context)
{
	{
		ScriptVariableData::CScopedSerializationConfig serializationConfig(archive);

		const SGUID guid = CScriptGraphNodeModel::GetNode().GetGraph().GetElement().GetGUID();
		serializationConfig.DeclareEnvDataTypes(guid);
		serializationConfig.DeclareScriptEnums(guid);
		serializationConfig.DeclareScriptStructs(guid);

		m_defaultValue.SerializeTypeId(archive);
	}
}

void CScriptGraphArrayRemoveByValueNode::RemapDependencies(IGUIDRemapper& guidRemapper)
{
	m_defaultValue.RemapDependencies(guidRemapper);
}

void CScriptGraphArrayRemoveByValueNode::Register(CScriptGraphNodeFactory& factory)
{
	class CCreator : public IScriptGraphNodeCreator
	{
	private:

		class CNodeCreationMenuCommand : public IScriptGraphNodeCreationMenuCommand
		{
		public:

			inline CNodeCreationMenuCommand(const SElementId& reference = SElementId())
				: m_reference(reference)
			{}

			// IMenuCommand

			IScriptGraphNodePtr Execute(const Vec2& pos)
			{
				return std::make_shared<CScriptGraphNode>(GetSchematycCore().CreateGUID(), stl::make_unique<CScriptGraphArrayRemoveByValueNode>(m_reference), pos);
			}

			// ~IMenuCommand

		private:

			SElementId m_reference;
		};

	public:

		// IScriptGraphNodeCreator

		virtual SGUID GetTypeGUID() const override
		{
			return CScriptGraphArrayRemoveByValueNode::ms_typeGUID;
		}

		virtual IScriptGraphNodePtr CreateNode(const SGUID& guid) override
		{
			return std::make_shared<CScriptGraphNode>(guid, stl::make_unique<CScriptGraphArrayRemoveByValueNode>());
		}

		virtual void PopulateNodeCreationMenu(IScriptGraphNodeCreationMenu& nodeCreationMenu, const IScriptView& scriptView, const IScriptGraph& graph) override
		{
			const char* szLabel = "Array::RemoveByValue";
			const char* szDescription = "Remove all elements of a specific value from array";

			nodeCreationMenu.AddOption(szLabel, szDescription, nullptr, std::make_shared<CNodeCreationMenuCommand>());

			// #SchematycTODO : This code is duplicated in all array nodes, find a way to reduce the duplication.

			auto visitEnvDataType = [&nodeCreationMenu, &scriptView, szLabel, szDescription](const IEnvDataType& envDataType) -> EVisitStatus
			{
				CStackString label;
				scriptView.QualifyName(envDataType, label);
				label.append("::");
				label.append(szLabel);
				nodeCreationMenu.AddOption(label.c_str(), szDescription, nullptr, std::make_shared<CNodeCreationMenuCommand>(SElementId(EDomain::Env, envDataType.GetGUID())));
				return EVisitStatus::Continue;
			};
			scriptView.VisitEnvDataTypes(EnvDataTypeConstVisitor::FromLambda(visitEnvDataType));
		}

		// ~IScriptGraphNodeCreator
	};

	factory.RegisterCreator(std::make_shared<CCreator>());
}

SRuntimeResult CScriptGraphArrayRemoveByValueNode::Execute(SRuntimeContext& context, const SRuntimeActivationParams& activationParams)
{
	CAnyArrayPtr pArray = DynamicCast<CAnyArrayPtr>(*context.node.GetInputData(EInputIdx::Array));
	CAnyConstRef value = *context.node.GetInputData(EInputIdx::Value);

	pArray->RemoveByValue(value);

	return SRuntimeResult(ERuntimeStatus::Continue, EOutputIdx::Out);
}

const SGUID CScriptGraphArrayRemoveByValueNode::ms_typeGUID = "aa5e9cf1-aba7-438a-904e-86174f5ba85c"_schematyc_guid;
} // Schematyc

SCHEMATYC_REGISTER_SCRIPT_GRAPH_NODE(Schematyc::CScriptGraphArrayRemoveByValueNode::Register)