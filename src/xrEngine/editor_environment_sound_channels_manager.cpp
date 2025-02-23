////////////////////////////////////////////////////////////////////////////
// Module : editor_environment_sound_channels_manager.cpp
// Created : 04.01.2008
// Modified : 04.01.2008
// Author : Dmitriy Iassenev
// Description : editor environment sound channels manager class
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#ifdef INGAME_EDITOR
#include "editor_environment_sound_channels_manager.hpp"
#include "property_collection.hpp"
#include "editor_environment_sound_channels_channel.hpp"
#include "editor_environment_detail.hpp"

using editor::environment::sound_channels::manager;
using editor::environment::sound_channels::channel;
using editor::environment::detail::logical_string_predicate;

template <>
void property_collection<manager::channel_container_type, manager>::display_name(u32 const& item_index, LPSTR const& buffer, u32 const& buffer_size)
{
	xr_strcpy(buffer, buffer_size, m_container[item_index]->id());
}

template <>
editor::property_holder* property_collection<manager::channel_container_type, manager>::create()
{
	channel* object = xr_new<channel>(m_holder, generate_unique_id("sound_channel_unique_id_").c_str());
	object->fill(this);
	return (object->object());
}

manager::manager() :
	m_collection(0),
	m_changed(true)
{
	m_collection = xr_new<collection_type>(&m_channels, this, &m_changed);
}

manager::~manager()
{
	xr_delete(m_collection);
	delete_data(m_channels);
	delete_data(m_channels_ids);
}

void manager::load()
{
	string_path file_name;
	CInifile* config =
		xr_new<CInifile>(
			FS.update_path(
				file_name,
				"$game_config$",
				"environment\\sound_channels.ltx"
			),
			TRUE,
			TRUE,
			FALSE
		);

	VERIFY(m_channels.empty());

	typedef CInifile::Root sections_type;
	sections_type& sections = config->sections();
	m_channels.reserve(sections.size());
	sections_type::const_iterator i = sections.begin();
	sections_type::const_iterator e = sections.end();
	for (; i != e; ++i)
	{
		channel* object = xr_new<channel>(*this, (*i)->Name);
		object->load(*config);
		object->fill(m_collection);
		m_channels.push_back(object);
	}

	xr_delete(config);
}

void manager::save()
{
	string_path file_name;
	CInifile* config =
		xr_new<CInifile>(
			FS.update_path(
				file_name,
				"$game_config$",
				"environment\\sound_channels.ltx"
			),
			FALSE,
			FALSE,
			TRUE
		);

	channel_container_type::iterator i = m_channels.begin();
	channel_container_type::iterator e = m_channels.end();
	for (; i != e; ++i)
		(*i)->save(*config);

	xr_delete(config);
}

void manager::fill(editor::property_holder* holder)
{
	VERIFY(holder);
	holder->add_property(
		"sound channels",
		"ambients",
		"this option is resposible for sound channels",
		m_collection
	);
}

manager::channels_ids_type const& manager::channels_ids() const
{
	if (!m_changed)
		return (m_channels_ids);

	m_changed = false;

	delete_data(m_channels_ids);

	m_channels_ids.resize(m_channels.size());

	channel_container_type::const_iterator i = m_channels.begin();
	channel_container_type::const_iterator e = m_channels.end();
	channels_ids_type::iterator j = m_channels_ids.begin();
	for (; i != e; ++i, ++j)
		*j = xr_strdup((*i)->id());

	m_channels_ids.sort(logical_string_predicate());

	return (m_channels_ids);
}

shared_str manager::unique_id(shared_str const& id) const
{
	if (m_collection->unique_id(id.c_str()))
		return (id);

	return (m_collection->generate_unique_id(id.c_str()));
}

#endif // #ifdef INGAME_EDITOR
