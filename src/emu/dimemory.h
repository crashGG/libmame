// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/***************************************************************************

    dimemory.h

    Device memory interfaces.

***************************************************************************/

#pragma once

#ifndef __EMU_H__
#error Dont include this file directly; include emu.h instead.
#endif

#ifndef MAME_EMU_DIMEMORY_H
#define MAME_EMU_DIMEMORY_H

#include <type_traits>

class device_memory_interface : public device_interface
{
	friend class device_scheduler;
	template <typename T, typename U> using is_related_class = std::bool_constant<std::is_convertible_v<std::add_pointer_t<T>, std::add_pointer_t<U> > >;
	template <typename T, typename U> using is_related_device = std::bool_constant<emu::detail::is_device_implementation<T>::value && is_related_class<T, U>::value >;
	template <typename T, typename U> using is_related_interface = std::bool_constant<emu::detail::is_device_interface<T>::value && is_related_class<T, U>::value >;
	template <typename T, typename U> using is_unrelated_device = std::bool_constant<emu::detail::is_device_implementation<T>::value && !is_related_class<T, U>::value >;
	template <typename T, typename U> using is_unrelated_interface = std::bool_constant<emu::detail::is_device_interface<T>::value && !is_related_class<T, U>::value >;

public:
	// Translation intentions for the translate() call
	enum {
		TR_READ,
		TR_WRITE,
		TR_FETCH
	};

	// construction/destruction
	device_memory_interface(const machine_config &mconfig, device_t &device);
	virtual ~device_memory_interface();

	// configuration access
	address_map_constructor get_addrmap(int spacenum = 0) const { return spacenum >= 0 && spacenum < int(m_address_map.size()) ? m_address_map[spacenum] : address_map_constructor(); }
	const address_space_config *space_config(int spacenum = 0) const { return spacenum >= 0 && spacenum < int(m_address_config.size()) ? m_address_config[spacenum] : nullptr; }
	int max_space_count() const { return m_address_config.size(); }

	// configuration helpers
	template <typename T, typename U, typename Ret, typename... Params>
	std::enable_if_t<is_related_device<T, U>::value> set_addrmap(int spacenum, T &obj, Ret (U::*func)(Params...)) { set_addrmap(spacenum, address_map_constructor(func, obj.tag(), &downcast<U &>(obj))); }
	template <typename T, typename U, typename Ret, typename... Params>
	std::enable_if_t<is_related_interface<T, U>::value> set_addrmap(int spacenum, T &obj, Ret (U::*func)(Params...)) { set_addrmap(spacenum, address_map_constructor(func, obj.device().tag(), &downcast<U &>(obj))); }
	template <typename T, typename U, typename Ret, typename... Params>
	std::enable_if_t<is_unrelated_device<T, U>::value> set_addrmap(int spacenum, T &obj, Ret (U::*func)(Params...)) { set_addrmap(spacenum, address_map_constructor(func, obj.tag(), &dynamic_cast<U &>(obj))); }
	template <typename T, typename U, typename Ret, typename... Params>
	std::enable_if_t<is_unrelated_interface<T, U>::value> set_addrmap(int spacenum, T &obj, Ret (U::*func)(Params...)) { set_addrmap(spacenum, address_map_constructor(func, obj.device().tag(), &dynamic_cast<U &>(obj))); }
	template <typename T, typename Ret, typename... Params>
	void set_addrmap(int spacenum, Ret (T::*func)(Params...));
	void set_addrmap(int spacenum, address_map_constructor map);

	// basic information getters
	bool has_space(int index = 0) const { return index >= 0 && index < int(m_addrspace.size()) && m_addrspace[index]; }
	bool has_configured_map(int index = 0) const { return index >= 0 && index < int(m_address_map.size()) && !m_address_map[index].isnull(); }
	address_space &space(int index = 0) const { assert(index >= 0 && index < int(m_addrspace.size()) && m_addrspace[index]); return *m_addrspace[index]; }
#if defined(__LIBRETRO__)
	int num_spaces() const { return int(m_addrspace.size()) ; }
#endif
	// address translation
	bool translate(int spacenum, int intention, offs_t &address, address_space *&target_space) { return memory_translate(spacenum, intention, address, target_space); }

	// deliberately ambiguous functions; if you have the memory interface
	// just use it
	device_memory_interface &memory() { return *this; }

	// setup functions - these are called in sequence for all device_memory_interface by the memory manager
	template <typename Space> void allocate(memory_manager &manager, int spacenum)
	{
		assert((0 <= spacenum) && (max_space_count() > spacenum));
		m_addrspace.resize(std::max<std::size_t>(m_addrspace.size(), spacenum + 1));
		assert(!m_addrspace[spacenum]);
		m_addrspace[spacenum] = std::make_unique<Space>(manager, *this, spacenum, space_config(spacenum)->addr_width());
	}
	void prepare_maps() { for (auto const &space : m_addrspace) { if (space) { space->prepare_map(); } } }
	void populate_from_maps() { for (auto const &space : m_addrspace) { if (space) { space->populate_from_map(); } } }
	void set_log_unmap(bool log) { for (auto const &space : m_addrspace) { if (space) { space->set_log_unmap(log); } } }

protected:
	using space_config_vector = std::vector<std::pair<int, const address_space_config *>>;

	// required overrides
	virtual space_config_vector memory_space_config() const = 0;

	// optional operation overrides
	virtual bool memory_translate(int spacenum, int intention, offs_t &address, address_space *&target_space);

	// interface-level overrides
	virtual void interface_config_complete() override;
	virtual void interface_validity_check(validity_checker &valid) const override;

	// configuration
	std::vector<address_map_constructor> m_address_map; // address maps for each address space

private:
	// internal state
	std::vector<const address_space_config *>   m_address_config; // configuration for each space
	std::vector<std::unique_ptr<address_space>> m_addrspace; // reported address spaces
};

// iterator
typedef device_interface_enumerator<device_memory_interface> memory_interface_enumerator;


#endif // MAME_EMU_DIMEMORY_H
