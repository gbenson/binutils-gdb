// i386.cc -- i386 target support for gold.

// Copyright 2006, 2007 Free Software Foundation, Inc.
// Written by Ian Lance Taylor <iant@google.com>.

// This file is part of gold.

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
// MA 02110-1301, USA.

#include "gold.h"

#include <cstring>

#include "elfcpp.h"
#include "parameters.h"
#include "reloc.h"
#include "i386.h"
#include "object.h"
#include "symtab.h"
#include "layout.h"
#include "output.h"
#include "target.h"
#include "target-reloc.h"
#include "target-select.h"
#include "tls.h"

namespace
{

using namespace gold;

class Output_data_plt_i386;

// The i386 target class.
// TLS info comes from
//   http://people.redhat.com/drepper/tls.pdf
//   http://www.lsd.ic.unicamp.br/~oliva/writeups/TLS/RFC-TLSDESC-x86.txt

class Target_i386 : public Sized_target<32, false>
{
 public:
  typedef Output_data_reloc<elfcpp::SHT_REL, true, 32, false> Reloc_section;

  Target_i386()
    : Sized_target<32, false>(&i386_info),
      got_(NULL), plt_(NULL), got_plt_(NULL), rel_dyn_(NULL),
      copy_relocs_(NULL), dynbss_(NULL)
  { }

  // Scan the relocations to look for symbol adjustments.
  void
  scan_relocs(const General_options& options,
	      Symbol_table* symtab,
	      Layout* layout,
	      Sized_relobj<32, false>* object,
	      unsigned int data_shndx,
	      unsigned int sh_type,
	      const unsigned char* prelocs,
	      size_t reloc_count,
	      size_t local_symbol_count,
	      const unsigned char* plocal_symbols,
	      Symbol** global_symbols);

  // Finalize the sections.
  void
  do_finalize_sections(Layout*);

  // Return the value to use for a dynamic which requires special
  // treatment.
  uint64_t
  do_dynsym_value(const Symbol*) const;

  // Relocate a section.
  void
  relocate_section(const Relocate_info<32, false>*,
		   unsigned int sh_type,
		   const unsigned char* prelocs,
		   size_t reloc_count,
		   unsigned char* view,
		   elfcpp::Elf_types<32>::Elf_Addr view_address,
		   off_t view_size);

  // Return a string used to fill a code section with nops.
  std::string
  do_code_fill(off_t length);

  // Return the size of the GOT section.
  off_t
  got_size()
  {
    gold_assert(this->got_ != NULL);
    return this->got_->data_size();
  }

 private:
  // The class which scans relocations.
  struct Scan
  {
    inline void
    local(const General_options& options, Symbol_table* symtab,
	  Layout* layout, Target_i386* target,
	  Sized_relobj<32, false>* object,
	  unsigned int data_shndx,
	  const elfcpp::Rel<32, false>& reloc, unsigned int r_type,
	  const elfcpp::Sym<32, false>& lsym);

    inline void
    global(const General_options& options, Symbol_table* symtab,
	   Layout* layout, Target_i386* target,
	   Sized_relobj<32, false>* object,
	   unsigned int data_shndx,
	   const elfcpp::Rel<32, false>& reloc, unsigned int r_type,
	   Symbol* gsym);

    static void
    unsupported_reloc_local(Sized_relobj<32, false>*, unsigned int r_type);

    static void
    unsupported_reloc_global(Sized_relobj<32, false>*, unsigned int r_type,
			     Symbol*);
  };

  // The class which implements relocation.
  class Relocate
  {
   public:
    Relocate()
      : skip_call_tls_get_addr_(false),
	local_dynamic_type_(LOCAL_DYNAMIC_NONE)
    { }

    ~Relocate()
    {
      if (this->skip_call_tls_get_addr_)
	{
	  // FIXME: This needs to specify the location somehow.
	  gold_error(_("missing expected TLS relocation"));
	}
    }

    // Do a relocation.  Return false if the caller should not issue
    // any warnings about this relocation.
    inline bool
    relocate(const Relocate_info<32, false>*, Target_i386*, size_t relnum,
	     const elfcpp::Rel<32, false>&,
	     unsigned int r_type, const Sized_symbol<32>*,
	     const Symbol_value<32>*,
	     unsigned char*, elfcpp::Elf_types<32>::Elf_Addr,
	     off_t);

   private:
    // Do a TLS relocation.
    inline void
    relocate_tls(const Relocate_info<32, false>*, size_t relnum,
		 const elfcpp::Rel<32, false>&,
		 unsigned int r_type, const Sized_symbol<32>*,
		 const Symbol_value<32>*,
		 unsigned char*, elfcpp::Elf_types<32>::Elf_Addr, off_t);

    // Do a TLS General-Dynamic to Local-Exec transition.
    inline void
    tls_gd_to_le(const Relocate_info<32, false>*, size_t relnum,
		 Output_segment* tls_segment,
		 const elfcpp::Rel<32, false>&, unsigned int r_type,
		 elfcpp::Elf_types<32>::Elf_Addr value,
		 unsigned char* view,
		 off_t view_size);

    // Do a TLS Local-Dynamic to Local-Exec transition.
    inline void
    tls_ld_to_le(const Relocate_info<32, false>*, size_t relnum,
		 Output_segment* tls_segment,
		 const elfcpp::Rel<32, false>&, unsigned int r_type,
		 elfcpp::Elf_types<32>::Elf_Addr value,
		 unsigned char* view,
		 off_t view_size);

    // Do a TLS Initial-Exec to Local-Exec transition.
    static inline void
    tls_ie_to_le(const Relocate_info<32, false>*, size_t relnum,
		 Output_segment* tls_segment,
		 const elfcpp::Rel<32, false>&, unsigned int r_type,
		 elfcpp::Elf_types<32>::Elf_Addr value,
		 unsigned char* view,
		 off_t view_size);

    // We need to keep track of which type of local dynamic relocation
    // we have seen, so that we can optimize R_386_TLS_LDO_32 correctly.
    enum Local_dynamic_type
    {
      LOCAL_DYNAMIC_NONE,
      LOCAL_DYNAMIC_SUN,
      LOCAL_DYNAMIC_GNU
    };

    // This is set if we should skip the next reloc, which should be a
    // PLT32 reloc against ___tls_get_addr.
    bool skip_call_tls_get_addr_;
    // The type of local dynamic relocation we have seen in the section
    // being relocated, if any.
    Local_dynamic_type local_dynamic_type_;
  };

  // Adjust TLS relocation type based on the options and whether this
  // is a local symbol.
  static tls::Tls_optimization
  optimize_tls_reloc(bool is_final, int r_type);

  // Get the GOT section, creating it if necessary.
  Output_data_got<32, false>*
  got_section(Symbol_table*, Layout*);

  // Get the GOT PLT section.
  Output_data_space*
  got_plt_section() const
  {
    gold_assert(this->got_plt_ != NULL);
    return this->got_plt_;
  }

  // Create a PLT entry for a global symbol.
  void
  make_plt_entry(Symbol_table*, Layout*, Symbol*);

  // Get the PLT section.
  const Output_data_plt_i386*
  plt_section() const
  {
    gold_assert(this->plt_ != NULL);
    return this->plt_;
  }

  // Get the dynamic reloc section, creating it if necessary.
  Reloc_section*
  rel_dyn_section(Layout*);

  // Copy a relocation against a global symbol.
  void
  copy_reloc(const General_options*, Symbol_table*, Layout*,
	     Sized_relobj<32, false>*, unsigned int,
	     Symbol*, const elfcpp::Rel<32, false>&);

  // Information about this specific target which we pass to the
  // general Target structure.
  static const Target::Target_info i386_info;

  // The GOT section.
  Output_data_got<32, false>* got_;
  // The PLT section.
  Output_data_plt_i386* plt_;
  // The GOT PLT section.
  Output_data_space* got_plt_;
  // The dynamic reloc section.
  Reloc_section* rel_dyn_;
  // Relocs saved to avoid a COPY reloc.
  Copy_relocs<32, false>* copy_relocs_;
  // Space for variables copied with a COPY reloc.
  Output_data_space* dynbss_;
};

const Target::Target_info Target_i386::i386_info =
{
  32,			// size
  false,		// is_big_endian
  elfcpp::EM_386,	// machine_code
  false,		// has_make_symbol
  false,		// has_resolve
  true,			// has_code_fill
  true,			// is_default_stack_executable
  "/usr/lib/libc.so.1",	// dynamic_linker
  0x08048000,		// default_text_segment_address
  0x1000,		// abi_pagesize
  0x1000		// common_pagesize
};

// Get the GOT section, creating it if necessary.

Output_data_got<32, false>*
Target_i386::got_section(Symbol_table* symtab, Layout* layout)
{
  if (this->got_ == NULL)
    {
      gold_assert(symtab != NULL && layout != NULL);

      this->got_ = new Output_data_got<32, false>();

      layout->add_output_section_data(".got", elfcpp::SHT_PROGBITS,
				      elfcpp::SHF_ALLOC | elfcpp::SHF_WRITE,
				      this->got_);

      // The old GNU linker creates a .got.plt section.  We just
      // create another set of data in the .got section.  Note that we
      // always create a PLT if we create a GOT, although the PLT
      // might be empty.
      this->got_plt_ = new Output_data_space(4);
      layout->add_output_section_data(".got", elfcpp::SHT_PROGBITS,
				      elfcpp::SHF_ALLOC | elfcpp::SHF_WRITE,
				      this->got_plt_);

      // The first three entries are reserved.
      this->got_plt_->set_space_size(3 * 4);

      // Define _GLOBAL_OFFSET_TABLE_ at the start of the PLT.
      symtab->define_in_output_data(this, "_GLOBAL_OFFSET_TABLE_", NULL,
				    this->got_plt_,
				    0, 0, elfcpp::STT_OBJECT,
				    elfcpp::STB_LOCAL,
				    elfcpp::STV_HIDDEN, 0,
				    false, false);
    }

  return this->got_;
}

// Get the dynamic reloc section, creating it if necessary.

Target_i386::Reloc_section*
Target_i386::rel_dyn_section(Layout* layout)
{
  if (this->rel_dyn_ == NULL)
    {
      gold_assert(layout != NULL);
      this->rel_dyn_ = new Reloc_section();
      layout->add_output_section_data(".rel.dyn", elfcpp::SHT_REL,
				      elfcpp::SHF_ALLOC, this->rel_dyn_);
    }
  return this->rel_dyn_;
}

// A class to handle the PLT data.

class Output_data_plt_i386 : public Output_section_data
{
 public:
  typedef Output_data_reloc<elfcpp::SHT_REL, true, 32, false> Reloc_section;

  Output_data_plt_i386(Layout*, Output_data_space*);

  // Add an entry to the PLT.
  void
  add_entry(Symbol* gsym);

  // Return the .rel.plt section data.
  const Reloc_section*
  rel_plt() const
  { return this->rel_; }

 protected:
  void
  do_adjust_output_section(Output_section* os);

 private:
  // The size of an entry in the PLT.
  static const int plt_entry_size = 16;

  // The first entry in the PLT for an executable.
  static unsigned char exec_first_plt_entry[plt_entry_size];

  // The first entry in the PLT for a shared object.
  static unsigned char dyn_first_plt_entry[plt_entry_size];

  // Other entries in the PLT for an executable.
  static unsigned char exec_plt_entry[plt_entry_size];

  // Other entries in the PLT for a shared object.
  static unsigned char dyn_plt_entry[plt_entry_size];

  // Set the final size.
  void
  do_set_address(uint64_t, off_t)
  { this->set_data_size((this->count_ + 1) * plt_entry_size); }

  // Write out the PLT data.
  void
  do_write(Output_file*);

  // The reloc section.
  Reloc_section* rel_;
  // The .got.plt section.
  Output_data_space* got_plt_;
  // The number of PLT entries.
  unsigned int count_;
};

// Create the PLT section.  The ordinary .got section is an argument,
// since we need to refer to the start.  We also create our own .got
// section just for PLT entries.

Output_data_plt_i386::Output_data_plt_i386(Layout* layout,
					   Output_data_space* got_plt)
  : Output_section_data(4), got_plt_(got_plt), count_(0)
{
  this->rel_ = new Reloc_section();
  layout->add_output_section_data(".rel.plt", elfcpp::SHT_REL,
				  elfcpp::SHF_ALLOC, this->rel_);
}

void
Output_data_plt_i386::do_adjust_output_section(Output_section* os)
{
  // UnixWare sets the entsize of .plt to 4, and so does the old GNU
  // linker, and so do we.
  os->set_entsize(4);
}

// Add an entry to the PLT.

void
Output_data_plt_i386::add_entry(Symbol* gsym)
{
  gold_assert(!gsym->has_plt_offset());

  // Note that when setting the PLT offset we skip the initial
  // reserved PLT entry.
  gsym->set_plt_offset((this->count_ + 1) * plt_entry_size);

  ++this->count_;

  off_t got_offset = this->got_plt_->data_size();

  // Every PLT entry needs a GOT entry which points back to the PLT
  // entry (this will be changed by the dynamic linker, normally
  // lazily when the function is called).
  this->got_plt_->set_space_size(got_offset + 4);

  // Every PLT entry needs a reloc.
  gsym->set_needs_dynsym_entry();
  this->rel_->add_global(gsym, elfcpp::R_386_JUMP_SLOT, this->got_plt_,
			 got_offset);

  // Note that we don't need to save the symbol.  The contents of the
  // PLT are independent of which symbols are used.  The symbols only
  // appear in the relocations.
}

// The first entry in the PLT for an executable.

unsigned char Output_data_plt_i386::exec_first_plt_entry[plt_entry_size] =
{
  0xff, 0x35,	// pushl contents of memory address
  0, 0, 0, 0,	// replaced with address of .got + 4
  0xff, 0x25,	// jmp indirect
  0, 0, 0, 0,	// replaced with address of .got + 8
  0, 0, 0, 0	// unused
};

// The first entry in the PLT for a shared object.

unsigned char Output_data_plt_i386::dyn_first_plt_entry[plt_entry_size] =
{
  0xff, 0xb3, 4, 0, 0, 0,	// pushl 4(%ebx)
  0xff, 0xa3, 8, 0, 0, 0,	// jmp *8(%ebx)
  0, 0, 0, 0			// unused
};

// Subsequent entries in the PLT for an executable.

unsigned char Output_data_plt_i386::exec_plt_entry[plt_entry_size] =
{
  0xff, 0x25,	// jmp indirect
  0, 0, 0, 0,	// replaced with address of symbol in .got
  0x68,		// pushl immediate
  0, 0, 0, 0,	// replaced with offset into relocation table
  0xe9,		// jmp relative
  0, 0, 0, 0	// replaced with offset to start of .plt
};

// Subsequent entries in the PLT for a shared object.

unsigned char Output_data_plt_i386::dyn_plt_entry[plt_entry_size] =
{
  0xff, 0xa3,	// jmp *offset(%ebx)
  0, 0, 0, 0,	// replaced with offset of symbol in .got
  0x68,		// pushl immediate
  0, 0, 0, 0,	// replaced with offset into relocation table
  0xe9,		// jmp relative
  0, 0, 0, 0	// replaced with offset to start of .plt
};

// Write out the PLT.  This uses the hand-coded instructions above,
// and adjusts them as needed.  This is all specified by the i386 ELF
// Processor Supplement.

void
Output_data_plt_i386::do_write(Output_file* of)
{
  const off_t offset = this->offset();
  const off_t oview_size = this->data_size();
  unsigned char* const oview = of->get_output_view(offset, oview_size);

  const off_t got_file_offset = this->got_plt_->offset();
  const off_t got_size = this->got_plt_->data_size();
  unsigned char* const got_view = of->get_output_view(got_file_offset,
						      got_size);

  unsigned char* pov = oview;

  elfcpp::Elf_types<32>::Elf_Addr plt_address = this->address();
  elfcpp::Elf_types<32>::Elf_Addr got_address = this->got_plt_->address();

  if (parameters->output_is_shared())
    memcpy(pov, dyn_first_plt_entry, plt_entry_size);
  else
    {
      memcpy(pov, exec_first_plt_entry, plt_entry_size);
      elfcpp::Swap_unaligned<32, false>::writeval(pov + 2, got_address + 4);
      elfcpp::Swap<32, false>::writeval(pov + 8, got_address + 8);
    }
  pov += plt_entry_size;

  unsigned char* got_pov = got_view;

  memset(got_pov, 0, 12);
  got_pov += 12;

  const int rel_size = elfcpp::Elf_sizes<32>::rel_size;

  unsigned int plt_offset = plt_entry_size;
  unsigned int plt_rel_offset = 0;
  unsigned int got_offset = 12;
  const unsigned int count = this->count_;
  for (unsigned int i = 0;
       i < count;
       ++i,
	 pov += plt_entry_size,
	 got_pov += 4,
	 plt_offset += plt_entry_size,
	 plt_rel_offset += rel_size,
	 got_offset += 4)
    {
      // Set and adjust the PLT entry itself.

      if (parameters->output_is_shared())
	{
	  memcpy(pov, dyn_plt_entry, plt_entry_size);
	  elfcpp::Swap_unaligned<32, false>::writeval(pov + 2, got_offset);
	}
      else
	{
	  memcpy(pov, exec_plt_entry, plt_entry_size);
	  elfcpp::Swap_unaligned<32, false>::writeval(pov + 2,
						      (got_address
						       + got_offset));
	}

      elfcpp::Swap_unaligned<32, false>::writeval(pov + 7, plt_rel_offset);
      elfcpp::Swap<32, false>::writeval(pov + 12,
					- (plt_offset + plt_entry_size));

      // Set the entry in the GOT.
      elfcpp::Swap<32, false>::writeval(got_pov, plt_address + plt_offset + 6);
    }

  gold_assert(pov - oview == oview_size);
  gold_assert(got_pov - got_view == got_size);

  of->write_output_view(offset, oview_size, oview);
  of->write_output_view(got_file_offset, got_size, got_view);
}

// Create a PLT entry for a global symbol.

void
Target_i386::make_plt_entry(Symbol_table* symtab, Layout* layout, Symbol* gsym)
{
  if (gsym->has_plt_offset())
    return;

  if (this->plt_ == NULL)
    {
      // Create the GOT sections first.
      this->got_section(symtab, layout);

      this->plt_ = new Output_data_plt_i386(layout, this->got_plt_);
      layout->add_output_section_data(".plt", elfcpp::SHT_PROGBITS,
				      (elfcpp::SHF_ALLOC
				       | elfcpp::SHF_EXECINSTR),
				      this->plt_);
    }

  this->plt_->add_entry(gsym);
}

// Handle a relocation against a non-function symbol defined in a
// dynamic object.  The traditional way to handle this is to generate
// a COPY relocation to copy the variable at runtime from the shared
// object into the executable's data segment.  However, this is
// undesirable in general, as if the size of the object changes in the
// dynamic object, the executable will no longer work correctly.  If
// this relocation is in a writable section, then we can create a
// dynamic reloc and the dynamic linker will resolve it to the correct
// address at runtime.  However, we do not want do that if the
// relocation is in a read-only section, as it would prevent the
// readonly segment from being shared.  And if we have to eventually
// generate a COPY reloc, then any dynamic relocations will be
// useless.  So this means that if this is a writable section, we need
// to save the relocation until we see whether we have to create a
// COPY relocation for this symbol for any other relocation.

void
Target_i386::copy_reloc(const General_options* options,
			Symbol_table* symtab,
			Layout* layout,
			Sized_relobj<32, false>* object,
			unsigned int data_shndx, Symbol* gsym,
			const elfcpp::Rel<32, false>& rel)
{
  Sized_symbol<32>* ssym;
  ssym = symtab->get_sized_symbol SELECT_SIZE_NAME(32) (gsym
							SELECT_SIZE(32));

  if (!Copy_relocs<32, false>::need_copy_reloc(options, object,
					       data_shndx, ssym))
    {
      // So far we do not need a COPY reloc.  Save this relocation.
      // If it turns out that we never need a COPY reloc for this
      // symbol, then we will emit the relocation.
      if (this->copy_relocs_ == NULL)
	this->copy_relocs_ = new Copy_relocs<32, false>();
      this->copy_relocs_->save(ssym, object, data_shndx, rel);
    }
  else
    {
      // Allocate space for this symbol in the .bss section.

      elfcpp::Elf_types<32>::Elf_WXword symsize = ssym->symsize();

      // There is no defined way to determine the required alignment
      // of the symbol.  We pick the alignment based on the size.  We
      // set an arbitrary maximum of 256.
      unsigned int align;
      for (align = 1; align < 512; align <<= 1)
	if ((symsize & align) != 0)
	  break;

      if (this->dynbss_ == NULL)
	{
	  this->dynbss_ = new Output_data_space(align);
	  layout->add_output_section_data(".bss",
					  elfcpp::SHT_NOBITS,
					  (elfcpp::SHF_ALLOC
					   | elfcpp::SHF_WRITE),
					  this->dynbss_);
	}

      Output_data_space* dynbss = this->dynbss_;

      if (align > dynbss->addralign())
	dynbss->set_space_alignment(align);

      off_t dynbss_size = dynbss->data_size();
      dynbss_size = align_address(dynbss_size, align);
      off_t offset = dynbss_size;
      dynbss->set_space_size(dynbss_size + symsize);

      symtab->define_with_copy_reloc(this, ssym, dynbss, offset);

      // Add the COPY reloc.
      Reloc_section* rel_dyn = this->rel_dyn_section(layout);
      rel_dyn->add_global(ssym, elfcpp::R_386_COPY, dynbss, offset);
    }
}

// Optimize the TLS relocation type based on what we know about the
// symbol.  IS_FINAL is true if the final address of this symbol is
// known at link time.

tls::Tls_optimization
Target_i386::optimize_tls_reloc(bool is_final, int r_type)
{
  // If we are generating a shared library, then we can't do anything
  // in the linker.
  if (parameters->output_is_shared())
    return tls::TLSOPT_NONE;

  switch (r_type)
    {
    case elfcpp::R_386_TLS_GD:
    case elfcpp::R_386_TLS_GOTDESC:
    case elfcpp::R_386_TLS_DESC_CALL:
      // These are General-Dynamic which permits fully general TLS
      // access.  Since we know that we are generating an executable,
      // we can convert this to Initial-Exec.  If we also know that
      // this is a local symbol, we can further switch to Local-Exec.
      if (is_final)
	return tls::TLSOPT_TO_LE;
      return tls::TLSOPT_TO_IE;

    case elfcpp::R_386_TLS_LDM:
      // This is Local-Dynamic, which refers to a local symbol in the
      // dynamic TLS block.  Since we know that we generating an
      // executable, we can switch to Local-Exec.
      return tls::TLSOPT_TO_LE;

    case elfcpp::R_386_TLS_LDO_32:
      // Another type of Local-Dynamic relocation.
      return tls::TLSOPT_TO_LE;

    case elfcpp::R_386_TLS_IE:
    case elfcpp::R_386_TLS_GOTIE:
    case elfcpp::R_386_TLS_IE_32:
      // These are Initial-Exec relocs which get the thread offset
      // from the GOT.  If we know that we are linking against the
      // local symbol, we can switch to Local-Exec, which links the
      // thread offset into the instruction.
      if (is_final)
	return tls::TLSOPT_TO_LE;
      return tls::TLSOPT_NONE;

    case elfcpp::R_386_TLS_LE:
    case elfcpp::R_386_TLS_LE_32:
      // When we already have Local-Exec, there is nothing further we
      // can do.
      return tls::TLSOPT_NONE;

    default:
      gold_unreachable();
    }
}

// Report an unsupported relocation against a local symbol.

void
Target_i386::Scan::unsupported_reloc_local(Sized_relobj<32, false>* object,
					   unsigned int r_type)
{
  gold_error(_("%s: unsupported reloc %u against local symbol"),
	     object->name().c_str(), r_type);
}

// Scan a relocation for a local symbol.

inline void
Target_i386::Scan::local(const General_options&,
			 Symbol_table* symtab,
			 Layout* layout,
			 Target_i386* target,
			 Sized_relobj<32, false>* object,
			 unsigned int data_shndx,
			 const elfcpp::Rel<32, false>& reloc,
			 unsigned int r_type,
			 const elfcpp::Sym<32, false>&)
{
  switch (r_type)
    {
    case elfcpp::R_386_NONE:
    case elfcpp::R_386_GNU_VTINHERIT:
    case elfcpp::R_386_GNU_VTENTRY:
      break;

    case elfcpp::R_386_32:
    case elfcpp::R_386_16:
    case elfcpp::R_386_8:
      // If building a shared library (or a position-independent
      // executable), we need to create a dynamic relocation for
      // this location. The relocation applied at link time will
      // apply the link-time value, so we flag the location with
      // an R_386_RELATIVE relocation so the dynamic loader can
      // relocate it easily.
      if (parameters->output_is_position_independent())
        {
	  // FIXME: R_386_RELATIVE only works for a 32-bit relocation.
	  gold_assert(r_type != elfcpp::R_386_16 && r_type != elfcpp::R_386_8);

          Reloc_section* rel_dyn = target->rel_dyn_section(layout);
          rel_dyn->add_local(object, 0, elfcpp::R_386_RELATIVE, data_shndx,
                             reloc.get_r_offset());
        }
      break;

    case elfcpp::R_386_PC32:
    case elfcpp::R_386_PC16:
    case elfcpp::R_386_PC8:
      break;

    case elfcpp::R_386_PLT32:
      // Since we know this is a local symbol, we can handle this as a
      // PC32 reloc.
      break;

    case elfcpp::R_386_GOTOFF:
    case elfcpp::R_386_GOTPC:
      // We need a GOT section.
      target->got_section(symtab, layout);
      break;

    case elfcpp::R_386_GOT32:
      {
        // The symbol requires a GOT entry.
        Output_data_got<32, false>* got = target->got_section(symtab, layout);
        unsigned int r_sym = elfcpp::elf_r_sym<32>(reloc.get_r_info());
        if (got->add_local(object, r_sym))
          {
            // If we are generating a shared object, we need to add a
            // dynamic RELATIVE relocation for this symbol.
            if (parameters->output_is_position_independent())
              {
                Reloc_section* rel_dyn = target->rel_dyn_section(layout);
                rel_dyn->add_local(object, 0, elfcpp::R_386_RELATIVE,
                                   data_shndx, reloc.get_r_offset());
              }
          }
      }
      break;

      // These are relocations which should only be seen by the
      // dynamic linker, and should never be seen here.
    case elfcpp::R_386_COPY:
    case elfcpp::R_386_GLOB_DAT:
    case elfcpp::R_386_JUMP_SLOT:
    case elfcpp::R_386_RELATIVE:
    case elfcpp::R_386_TLS_TPOFF:
    case elfcpp::R_386_TLS_DTPMOD32:
    case elfcpp::R_386_TLS_DTPOFF32:
    case elfcpp::R_386_TLS_TPOFF32:
    case elfcpp::R_386_TLS_DESC:
      gold_error(_("%s: unexpected reloc %u in object file"),
		 object->name().c_str(), r_type);
      break;

      // These are initial TLS relocs, which are expected when
      // linking.
    case elfcpp::R_386_TLS_GD:            // Global-dynamic
    case elfcpp::R_386_TLS_GOTDESC:       // Global-dynamic (from ~oliva url)
    case elfcpp::R_386_TLS_DESC_CALL:
    case elfcpp::R_386_TLS_LDM:           // Local-dynamic
    case elfcpp::R_386_TLS_LDO_32:        // Alternate local-dynamic
    case elfcpp::R_386_TLS_IE:            // Initial-exec
    case elfcpp::R_386_TLS_IE_32:
    case elfcpp::R_386_TLS_GOTIE:
    case elfcpp::R_386_TLS_LE:            // Local-exec
    case elfcpp::R_386_TLS_LE_32:
      {
	bool output_is_shared = parameters->output_is_shared();
	const tls::Tls_optimization optimized_type
            = Target_i386::optimize_tls_reloc(!output_is_shared, r_type);
	switch (r_type)
	  {
	  case elfcpp::R_386_TLS_GD:          // Global-dynamic
	  case elfcpp::R_386_TLS_GOTDESC:     // Global-dynamic (from ~oliva)
	  case elfcpp::R_386_TLS_DESC_CALL:
	    // FIXME: If not relaxing to LE, we need to generate
	    // DTPMOD32 and DTPOFF32 relocs.
	    if (optimized_type != tls::TLSOPT_TO_LE)
	      unsupported_reloc_local(object, r_type);
	    break;

	  case elfcpp::R_386_TLS_LDM:         // Local-dynamic
	    // FIXME: If not relaxing to LE, we need to generate a
	    // DTPMOD32 reloc.
	    if (optimized_type != tls::TLSOPT_TO_LE)
	      unsupported_reloc_local(object, r_type);
	    break;

	  case elfcpp::R_386_TLS_LDO_32:      // Alternate local-dynamic
	    break;

	  case elfcpp::R_386_TLS_IE:          // Initial-exec
	  case elfcpp::R_386_TLS_IE_32:
	  case elfcpp::R_386_TLS_GOTIE:
	    // FIXME: If not relaxing to LE, we need to generate a
	    // TPOFF or TPOFF32 reloc.
	    if (optimized_type != tls::TLSOPT_TO_LE)
	      unsupported_reloc_local(object, r_type);
	    break;

	  case elfcpp::R_386_TLS_LE:          // Local-exec
	  case elfcpp::R_386_TLS_LE_32:
	    // FIXME: If generating a shared object, we need to copy
	    // this relocation into the object.
	    gold_assert(!output_is_shared);
	    break;

	  default:
	    gold_unreachable();
	  }
      }
      break;

    case elfcpp::R_386_32PLT:
    case elfcpp::R_386_TLS_GD_32:
    case elfcpp::R_386_TLS_GD_PUSH:
    case elfcpp::R_386_TLS_GD_CALL:
    case elfcpp::R_386_TLS_GD_POP:
    case elfcpp::R_386_TLS_LDM_32:
    case elfcpp::R_386_TLS_LDM_PUSH:
    case elfcpp::R_386_TLS_LDM_CALL:
    case elfcpp::R_386_TLS_LDM_POP:
    case elfcpp::R_386_USED_BY_INTEL_200:
    default:
      unsupported_reloc_local(object, r_type);
      break;
    }
}

// Report an unsupported relocation against a global symbol.

void
Target_i386::Scan::unsupported_reloc_global(Sized_relobj<32, false>* object,
					    unsigned int r_type,
					    Symbol* gsym)
{
  gold_error(_("%s: unsupported reloc %u against global symbol %s"),
	     object->name().c_str(), r_type, gsym->name());
}

// Scan a relocation for a global symbol.

inline void
Target_i386::Scan::global(const General_options& options,
			  Symbol_table* symtab,
			  Layout* layout,
			  Target_i386* target,
			  Sized_relobj<32, false>* object,
			  unsigned int data_shndx,
			  const elfcpp::Rel<32, false>& reloc,
			  unsigned int r_type,
			  Symbol* gsym)
{
  switch (r_type)
    {
    case elfcpp::R_386_NONE:
    case elfcpp::R_386_GNU_VTINHERIT:
    case elfcpp::R_386_GNU_VTENTRY:
      break;

    case elfcpp::R_386_32:
    case elfcpp::R_386_PC32:
    case elfcpp::R_386_16:
    case elfcpp::R_386_PC16:
    case elfcpp::R_386_8:
    case elfcpp::R_386_PC8:
      {
        bool is_pcrel = (r_type == elfcpp::R_386_PC32
		         || r_type == elfcpp::R_386_PC16
		         || r_type == elfcpp::R_386_PC8);

        if (gsym->is_from_dynobj()
            || (parameters->output_is_shared()
                && gsym->is_preemptible()))
	  {
	    // (a) This symbol is defined in a dynamic object.  If it is a
	    // function, we make a PLT entry.  Otherwise we need to
	    // either generate a COPY reloc or copy this reloc.
	    // (b) We are building a shared object and this symbol is
	    // preemptible. If it is a function, we make a PLT entry.
	    // Otherwise, we copy the reloc.
	    if (gsym->type() == elfcpp::STT_FUNC)
	      {
	        target->make_plt_entry(symtab, layout, gsym);
  
	        // If this is not a PC relative reference, then we may
	        // be taking the address of the function.  In that case
	        // we need to set the entry in the dynamic symbol table
	        // to the address of the PLT entry. We will also need to
	        // create a dynamic relocation.
	        if (!is_pcrel)
		  {
		    if (gsym->is_from_dynobj())
		      gsym->set_needs_dynsym_value();
                    if (parameters->output_is_position_independent())
                      {
			// FIXME: If this is an 8-bit or 16-bit
			// relocation, R_386_RELATIVE won't work.
			gold_assert(r_type != elfcpp::R_386_16
				    && r_type != elfcpp::R_386_8);

                        Reloc_section* rel_dyn =
                          target->rel_dyn_section(layout);
                        rel_dyn->add_local(object, 0, elfcpp::R_386_RELATIVE,
                                           data_shndx, reloc.get_r_offset());
                      }
		  }
	      }
	    else if (parameters->output_is_shared())
	      {
	        // We do not make COPY relocs in shared objects.
                Reloc_section* rel_dyn = target->rel_dyn_section(layout);
                rel_dyn->add_global(gsym, r_type, object, data_shndx, 
                                    reloc.get_r_offset());
	      }
	    else
	      target->copy_reloc(&options, symtab, layout, object, data_shndx,
			         gsym, reloc);
	  }
        else if (!is_pcrel && parameters->output_is_position_independent())
          {
	    // FIXME: If this is an 8-bit or 16-bit relocation,
	    // R_386_RELATIVE won't work.
	    gold_assert(r_type != elfcpp::R_386_16
			&& r_type != elfcpp::R_386_8);

            // This is not a PC-relative reference, so we need to generate
            // a dynamic relocation.
            Reloc_section* rel_dyn = target->rel_dyn_section(layout);
            rel_dyn->add_local(object, 0, elfcpp::R_386_RELATIVE, data_shndx,
                               reloc.get_r_offset());
          }
      }
      break;

    case elfcpp::R_386_GOT32:
      {
        // The symbol requires a GOT entry.
        Output_data_got<32, false>* got = target->got_section(symtab, layout);
        if (got->add_global(gsym))
	  {
            // If this symbol is not fully resolved, we need to add a
            // dynamic relocation for it.
            if (!gsym->final_value_is_known())
              {
                Reloc_section* rel_dyn = target->rel_dyn_section(layout);
                rel_dyn->add_global(gsym, elfcpp::R_386_GLOB_DAT, got,
                                    gsym->got_offset());
              }
          }
      }
      break;

    case elfcpp::R_386_PLT32:
      // If the symbol is fully resolved, this is just a PC32 reloc.
      // Otherwise we need a PLT entry.
      if (gsym->final_value_is_known())
	break;
      // If building a shared library, we can also skip the PLT entry
      // if the symbol is defined in the output file and is protected
      // or hidden.
      if (gsym->is_defined()
          && !gsym->is_from_dynobj()
          && !gsym->is_preemptible())
	break;
      target->make_plt_entry(symtab, layout, gsym);
      break;

    case elfcpp::R_386_GOTOFF:
    case elfcpp::R_386_GOTPC:
      // We need a GOT section.
      target->got_section(symtab, layout);
      break;

      // These are relocations which should only be seen by the
      // dynamic linker, and should never be seen here.
    case elfcpp::R_386_COPY:
    case elfcpp::R_386_GLOB_DAT:
    case elfcpp::R_386_JUMP_SLOT:
    case elfcpp::R_386_RELATIVE:
    case elfcpp::R_386_TLS_TPOFF:
    case elfcpp::R_386_TLS_DTPMOD32:
    case elfcpp::R_386_TLS_DTPOFF32:
    case elfcpp::R_386_TLS_TPOFF32:
    case elfcpp::R_386_TLS_DESC:
      gold_error(_("%s: unexpected reloc %u in object file"),
		 object->name().c_str(), r_type);
      break;

      // These are initial tls relocs, which are expected when
      // linking.
    case elfcpp::R_386_TLS_GD:            // Global-dynamic
    case elfcpp::R_386_TLS_GOTDESC:       // Global-dynamic (from ~oliva url)
    case elfcpp::R_386_TLS_DESC_CALL:
    case elfcpp::R_386_TLS_LDM:           // Local-dynamic
    case elfcpp::R_386_TLS_LDO_32:        // Alternate local-dynamic
    case elfcpp::R_386_TLS_IE:            // Initial-exec
    case elfcpp::R_386_TLS_IE_32:
    case elfcpp::R_386_TLS_GOTIE:
    case elfcpp::R_386_TLS_LE:            // Local-exec
    case elfcpp::R_386_TLS_LE_32:
      {
	const bool is_final = gsym->final_value_is_known();
	const tls::Tls_optimization optimized_type
            = Target_i386::optimize_tls_reloc(is_final, r_type);
	switch (r_type)
	  {
	  case elfcpp::R_386_TLS_GD:          // Global-dynamic
	  case elfcpp::R_386_TLS_GOTDESC:     // Global-dynamic (~oliva url)
	  case elfcpp::R_386_TLS_DESC_CALL:
	    // FIXME: If not relaxing to LE, we need to generate
	    // DTPMOD32 and DTPOFF32 relocs.
	    if (optimized_type != tls::TLSOPT_TO_LE)
	      unsupported_reloc_global(object, r_type, gsym);
	    break;

	  case elfcpp::R_386_TLS_LDM:         // Local-dynamic
	    // FIXME: If not relaxing to LE, we need to generate a
	    // DTPMOD32 reloc.
	    if (optimized_type != tls::TLSOPT_TO_LE)
	      unsupported_reloc_global(object, r_type, gsym);
	    break;

	  case elfcpp::R_386_TLS_LDO_32:      // Alternate local-dynamic
	    break;

	  case elfcpp::R_386_TLS_IE:          // Initial-exec
	  case elfcpp::R_386_TLS_IE_32:
	  case elfcpp::R_386_TLS_GOTIE:
	    // FIXME: If not relaxing to LE, we need to generate a
	    // TPOFF or TPOFF32 reloc.
	    if (optimized_type != tls::TLSOPT_TO_LE)
	      unsupported_reloc_global(object, r_type, gsym);
	    break;

	  case elfcpp::R_386_TLS_LE:          // Local-exec
	  case elfcpp::R_386_TLS_LE_32:
	    // FIXME: If generating a shared object, we need to copy
	    // this relocation into the object.
	    gold_assert(!parameters->output_is_shared());
	    break;

	  default:
	    gold_unreachable();
	  }
      }
      break;

    case elfcpp::R_386_32PLT:
    case elfcpp::R_386_TLS_GD_32:
    case elfcpp::R_386_TLS_GD_PUSH:
    case elfcpp::R_386_TLS_GD_CALL:
    case elfcpp::R_386_TLS_GD_POP:
    case elfcpp::R_386_TLS_LDM_32:
    case elfcpp::R_386_TLS_LDM_PUSH:
    case elfcpp::R_386_TLS_LDM_CALL:
    case elfcpp::R_386_TLS_LDM_POP:
    case elfcpp::R_386_USED_BY_INTEL_200:
    default:
      unsupported_reloc_global(object, r_type, gsym);
      break;
    }
}

// Scan relocations for a section.

void
Target_i386::scan_relocs(const General_options& options,
			 Symbol_table* symtab,
			 Layout* layout,
			 Sized_relobj<32, false>* object,
			 unsigned int data_shndx,
			 unsigned int sh_type,
			 const unsigned char* prelocs,
			 size_t reloc_count,
			 size_t local_symbol_count,
			 const unsigned char* plocal_symbols,
			 Symbol** global_symbols)
{
  if (sh_type == elfcpp::SHT_RELA)
    {
      gold_error(_("%s: unsupported RELA reloc section"),
		 object->name().c_str());
      return;
    }

  gold::scan_relocs<32, false, Target_i386, elfcpp::SHT_REL,
		    Target_i386::Scan>(
    options,
    symtab,
    layout,
    this,
    object,
    data_shndx,
    prelocs,
    reloc_count,
    local_symbol_count,
    plocal_symbols,
    global_symbols);
}

// Finalize the sections.

void
Target_i386::do_finalize_sections(Layout* layout)
{
  // Fill in some more dynamic tags.
  Output_data_dynamic* const odyn = layout->dynamic_data();
  if (odyn != NULL)
    {
      if (this->got_plt_ != NULL)
	odyn->add_section_address(elfcpp::DT_PLTGOT, this->got_plt_);

      if (this->plt_ != NULL)
	{
	  const Output_data* od = this->plt_->rel_plt();
	  odyn->add_section_size(elfcpp::DT_PLTRELSZ, od);
	  odyn->add_section_address(elfcpp::DT_JMPREL, od);
	  odyn->add_constant(elfcpp::DT_PLTREL, elfcpp::DT_REL);
	}

      if (this->rel_dyn_ != NULL)
	{
	  const Output_data* od = this->rel_dyn_;
	  odyn->add_section_address(elfcpp::DT_REL, od);
	  odyn->add_section_size(elfcpp::DT_RELSZ, od);
	  odyn->add_constant(elfcpp::DT_RELENT,
			     elfcpp::Elf_sizes<32>::rel_size);
	}

      if (!parameters->output_is_shared())
	{
	  // The value of the DT_DEBUG tag is filled in by the dynamic
	  // linker at run time, and used by the debugger.
	  odyn->add_constant(elfcpp::DT_DEBUG, 0);
	}
    }

  // Emit any relocs we saved in an attempt to avoid generating COPY
  // relocs.
  if (this->copy_relocs_ == NULL)
    return;
  if (this->copy_relocs_->any_to_emit())
    {
      Reloc_section* rel_dyn = this->rel_dyn_section(layout);
      this->copy_relocs_->emit(rel_dyn);
    }
  delete this->copy_relocs_;
  this->copy_relocs_ = NULL;
}

// Perform a relocation.

inline bool
Target_i386::Relocate::relocate(const Relocate_info<32, false>* relinfo,
				Target_i386* target,
				size_t relnum,
				const elfcpp::Rel<32, false>& rel,
				unsigned int r_type,
				const Sized_symbol<32>* gsym,
				const Symbol_value<32>* psymval,
				unsigned char* view,
				elfcpp::Elf_types<32>::Elf_Addr address,
				off_t view_size)
{
  if (this->skip_call_tls_get_addr_)
    {
      if (r_type != elfcpp::R_386_PLT32
	  || gsym == NULL
	  || strcmp(gsym->name(), "___tls_get_addr") != 0)
	gold_error_at_location(relinfo, relnum, rel.get_r_offset(),
			       _("missing expected TLS relocation"));
      else
	{
	  this->skip_call_tls_get_addr_ = false;
	  return false;
	}
    }

  // Pick the value to use for symbols defined in shared objects.
  Symbol_value<32> symval;
  if (gsym != NULL
      && (gsym->is_from_dynobj()
          || (parameters->output_is_shared()
              && gsym->is_preemptible()))
      && gsym->has_plt_offset())
    {
      symval.set_output_value(target->plt_section()->address()
			      + gsym->plt_offset());
      psymval = &symval;
    }

  const Sized_relobj<32, false>* object = relinfo->object;

  // Get the GOT offset if needed.
  // The GOT pointer points to the end of the GOT section.
  // We need to subtract the size of the GOT section to get
  // the actual offset to use in the relocation.
  bool have_got_offset = false;
  unsigned int got_offset = 0;
  switch (r_type)
    {
    case elfcpp::R_386_GOT32:
      if (gsym != NULL)
        {
          gold_assert(gsym->has_got_offset());
          got_offset = gsym->got_offset() - target->got_size();
        }
      else
        {
          unsigned int r_sym = elfcpp::elf_r_sym<32>(rel.get_r_info());
          got_offset = object->local_got_offset(r_sym) - target->got_size();
        }
      have_got_offset = true;
      break;

    default:
      break;
    }

  switch (r_type)
    {
    case elfcpp::R_386_NONE:
    case elfcpp::R_386_GNU_VTINHERIT:
    case elfcpp::R_386_GNU_VTENTRY:
      break;

    case elfcpp::R_386_32:
      Relocate_functions<32, false>::rel32(view, object, psymval);
      break;

    case elfcpp::R_386_PC32:
      Relocate_functions<32, false>::pcrel32(view, object, psymval, address);
      break;

    case elfcpp::R_386_16:
      Relocate_functions<32, false>::rel16(view, object, psymval);
      break;

    case elfcpp::R_386_PC16:
      Relocate_functions<32, false>::pcrel16(view, object, psymval, address);
      break;

    case elfcpp::R_386_8:
      Relocate_functions<32, false>::rel8(view, object, psymval);
      break;

    case elfcpp::R_386_PC8:
      Relocate_functions<32, false>::pcrel8(view, object, psymval, address);
      break;

    case elfcpp::R_386_PLT32:
      gold_assert(gsym == NULL
		  || gsym->has_plt_offset()
		  || gsym->final_value_is_known());
      Relocate_functions<32, false>::pcrel32(view, object, psymval, address);
      break;

    case elfcpp::R_386_GOT32:
      gold_assert(have_got_offset);
      Relocate_functions<32, false>::rel32(view, got_offset);
      break;

    case elfcpp::R_386_GOTOFF:
      {
	elfcpp::Elf_types<32>::Elf_Addr value;
	value = (psymval->value(object, 0)
		 - target->got_plt_section()->address());
	Relocate_functions<32, false>::rel32(view, value);
      }
      break;

    case elfcpp::R_386_GOTPC:
      {
	elfcpp::Elf_types<32>::Elf_Addr value;
	value = target->got_plt_section()->address();
	Relocate_functions<32, false>::pcrel32(view, value, address);
      }
      break;

    case elfcpp::R_386_COPY:
    case elfcpp::R_386_GLOB_DAT:
    case elfcpp::R_386_JUMP_SLOT:
    case elfcpp::R_386_RELATIVE:
      // These are outstanding tls relocs, which are unexpected when
      // linking.
    case elfcpp::R_386_TLS_TPOFF:
    case elfcpp::R_386_TLS_DTPMOD32:
    case elfcpp::R_386_TLS_DTPOFF32:
    case elfcpp::R_386_TLS_TPOFF32:
    case elfcpp::R_386_TLS_DESC:
      gold_error_at_location(relinfo, relnum, rel.get_r_offset(),
			     _("unexpected reloc %u in object file"),
			     r_type);
      break;

      // These are initial tls relocs, which are expected when
      // linking.
    case elfcpp::R_386_TLS_GD:             // Global-dynamic
    case elfcpp::R_386_TLS_GOTDESC:        // Global-dynamic (from ~oliva url)
    case elfcpp::R_386_TLS_DESC_CALL:
    case elfcpp::R_386_TLS_LDM:            // Local-dynamic
    case elfcpp::R_386_TLS_LDO_32:         // Alternate local-dynamic
    case elfcpp::R_386_TLS_IE:             // Initial-exec
    case elfcpp::R_386_TLS_IE_32:
    case elfcpp::R_386_TLS_GOTIE:
    case elfcpp::R_386_TLS_LE:             // Local-exec
    case elfcpp::R_386_TLS_LE_32:
      this->relocate_tls(relinfo, relnum, rel, r_type, gsym, psymval, view,
			 address, view_size);
      break;

    case elfcpp::R_386_32PLT:
    case elfcpp::R_386_TLS_GD_32:
    case elfcpp::R_386_TLS_GD_PUSH:
    case elfcpp::R_386_TLS_GD_CALL:
    case elfcpp::R_386_TLS_GD_POP:
    case elfcpp::R_386_TLS_LDM_32:
    case elfcpp::R_386_TLS_LDM_PUSH:
    case elfcpp::R_386_TLS_LDM_CALL:
    case elfcpp::R_386_TLS_LDM_POP:
    case elfcpp::R_386_USED_BY_INTEL_200:
    default:
      gold_error_at_location(relinfo, relnum, rel.get_r_offset(),
			     _("unsupported reloc %u"),
			     r_type);
      break;
    }

  return true;
}

// Perform a TLS relocation.

inline void
Target_i386::Relocate::relocate_tls(const Relocate_info<32, false>* relinfo,
				    size_t relnum,
				    const elfcpp::Rel<32, false>& rel,
				    unsigned int r_type,
				    const Sized_symbol<32>* gsym,
				    const Symbol_value<32>* psymval,
				    unsigned char* view,
				    elfcpp::Elf_types<32>::Elf_Addr,
				    off_t view_size)
{
  Output_segment* tls_segment = relinfo->layout->tls_segment();
  if (tls_segment == NULL)
    {
      gold_error_at_location(relinfo, relnum, rel.get_r_offset(),
			     _("TLS reloc but no TLS segment"));
      return;
    }

  elfcpp::Elf_types<32>::Elf_Addr value = psymval->value(relinfo->object, 0);

  const bool is_final = (gsym == NULL
			 ? !parameters->output_is_position_independent()
			 : gsym->final_value_is_known());
  const tls::Tls_optimization optimized_type
      = Target_i386::optimize_tls_reloc(is_final, r_type);
  switch (r_type)
    {
    case elfcpp::R_386_TLS_GD:           // Global-dynamic
      if (optimized_type == tls::TLSOPT_TO_LE)
	{
	  this->tls_gd_to_le(relinfo, relnum, tls_segment,
			     rel, r_type, value, view,
			     view_size);
	  break;
	}
      gold_error_at_location(relinfo, relnum, rel.get_r_offset(),
			     _("unsupported reloc %u"),
			     r_type);
      break;

    case elfcpp::R_386_TLS_GOTDESC:      // Global-dynamic (from ~oliva url)
    case elfcpp::R_386_TLS_DESC_CALL:
      gold_error_at_location(relinfo, relnum, rel.get_r_offset(),
			     _("unsupported reloc %u"),
			     r_type);
      break;

    case elfcpp::R_386_TLS_LDM:          // Local-dynamic
      if (this->local_dynamic_type_ == LOCAL_DYNAMIC_SUN)
	{
	  gold_error_at_location(relinfo, relnum, rel.get_r_offset(),
				 _("both SUN and GNU model "
				   "TLS relocations"));
	  break;
	}
      this->local_dynamic_type_ = LOCAL_DYNAMIC_GNU;
      if (optimized_type == tls::TLSOPT_TO_LE)
	{
	  this->tls_ld_to_le(relinfo, relnum, tls_segment, rel, r_type,
			     value, view, view_size);
	  break;
	}
      gold_error_at_location(relinfo, relnum, rel.get_r_offset(),
			     _("unsupported reloc %u"),
			     r_type);
      break;

    case elfcpp::R_386_TLS_LDO_32:       // Alternate local-dynamic
      // This reloc can appear in debugging sections, in which case we
      // won't see the TLS_LDM reloc.  The local_dynamic_type field
      // tells us this.
      if (optimized_type != tls::TLSOPT_TO_LE
	  || this->local_dynamic_type_ == LOCAL_DYNAMIC_NONE)
	value = value - tls_segment->vaddr();
      else if (this->local_dynamic_type_ == LOCAL_DYNAMIC_GNU)
	value = value - (tls_segment->vaddr() + tls_segment->memsz());
      else
	value = tls_segment->vaddr() + tls_segment->memsz() - value;
      Relocate_functions<32, false>::rel32(view, value);
      break;

    case elfcpp::R_386_TLS_IE:           // Initial-exec
    case elfcpp::R_386_TLS_GOTIE:
    case elfcpp::R_386_TLS_IE_32:
      if (optimized_type == tls::TLSOPT_TO_LE)
	{
	  Target_i386::Relocate::tls_ie_to_le(relinfo, relnum, tls_segment,
					      rel, r_type, value, view,
					      view_size);
	  break;
	}
      gold_error_at_location(relinfo, relnum, rel.get_r_offset(),
			     _("unsupported reloc %u"),
			     r_type);
      break;

    case elfcpp::R_386_TLS_LE:           // Local-exec
      value = value - (tls_segment->vaddr() + tls_segment->memsz());
      Relocate_functions<32, false>::rel32(view, value);
      break;

    case elfcpp::R_386_TLS_LE_32:
      value = tls_segment->vaddr() + tls_segment->memsz() - value;
      Relocate_functions<32, false>::rel32(view, value);
      break;
    }
}

// Do a relocation in which we convert a TLS General-Dynamic to a
// Local-Exec.

inline void
Target_i386::Relocate::tls_gd_to_le(const Relocate_info<32, false>* relinfo,
				    size_t relnum,
				    Output_segment* tls_segment,
				    const elfcpp::Rel<32, false>& rel,
				    unsigned int,
				    elfcpp::Elf_types<32>::Elf_Addr value,
				    unsigned char* view,
				    off_t view_size)
{
  // leal foo(,%reg,1),%eax; call ___tls_get_addr
  //  ==> movl %gs:0,%eax; subl $foo@tpoff,%eax
  // leal foo(%reg),%eax; call ___tls_get_addr
  //  ==> movl %gs:0,%eax; subl $foo@tpoff,%eax

  tls::check_range(relinfo, relnum, rel.get_r_offset(), view_size, -2);
  tls::check_range(relinfo, relnum, rel.get_r_offset(), view_size, 9);

  unsigned char op1 = view[-1];
  unsigned char op2 = view[-2];

  tls::check_tls(relinfo, relnum, rel.get_r_offset(),
                 op2 == 0x8d || op2 == 0x04);
  tls::check_tls(relinfo, relnum, rel.get_r_offset(), view[4] == 0xe8);

  int roff = 5;

  if (op2 == 0x04)
    {
      tls::check_range(relinfo, relnum, rel.get_r_offset(), view_size, -3);
      tls::check_tls(relinfo, relnum, rel.get_r_offset(), view[-3] == 0x8d);
      tls::check_tls(relinfo, relnum, rel.get_r_offset(),
                     ((op1 & 0xc7) == 0x05 && op1 != (4 << 3)));
      memcpy(view - 3, "\x65\xa1\0\0\0\0\x81\xe8\0\0\0", 12);
    }
  else
    {
      tls::check_tls(relinfo, relnum, rel.get_r_offset(),
                     (op1 & 0xf8) == 0x80 && (op1 & 7) != 4);
      if (static_cast<off_t>(rel.get_r_offset() + 9) < view_size
          && view[9] == 0x90)
	{
	  // There is a trailing nop.  Use the size byte subl.
	  memcpy(view - 2, "\x65\xa1\0\0\0\0\x81\xe8\0\0\0", 12);
	  roff = 6;
	}
      else
	{
	  // Use the five byte subl.
	  memcpy(view - 2, "\x65\xa1\0\0\0\0\x2d\0\0\0", 11);
	}
    }

  value = tls_segment->vaddr() + tls_segment->memsz() - value;
  Relocate_functions<32, false>::rel32(view + roff, value);

  // The next reloc should be a PLT32 reloc against __tls_get_addr.
  // We can skip it.
  this->skip_call_tls_get_addr_ = true;
}

// Do a relocation in which we convert a TLS Local-Dynamic to a
// Local-Exec.

inline void
Target_i386::Relocate::tls_ld_to_le(const Relocate_info<32, false>* relinfo,
				    size_t relnum,
				    Output_segment*,
				    const elfcpp::Rel<32, false>& rel,
				    unsigned int,
				    elfcpp::Elf_types<32>::Elf_Addr,
				    unsigned char* view,
				    off_t view_size)
{
  // leal foo(%reg), %eax; call ___tls_get_addr
  // ==> movl %gs:0,%eax; nop; leal 0(%esi,1),%esi

  tls::check_range(relinfo, relnum, rel.get_r_offset(), view_size, -2);
  tls::check_range(relinfo, relnum, rel.get_r_offset(), view_size, 9);

  // FIXME: Does this test really always pass?
  tls::check_tls(relinfo, relnum, rel.get_r_offset(),
                 view[-2] == 0x8d && view[-1] == 0x83);

  tls::check_tls(relinfo, relnum, rel.get_r_offset(), view[4] == 0xe8);

  memcpy(view - 2, "\x65\xa1\0\0\0\0\x90\x8d\x74\x26\0", 11);

  // The next reloc should be a PLT32 reloc against __tls_get_addr.
  // We can skip it.
  this->skip_call_tls_get_addr_ = true;
}

// Do a relocation in which we convert a TLS Initial-Exec to a
// Local-Exec.

inline void
Target_i386::Relocate::tls_ie_to_le(const Relocate_info<32, false>* relinfo,
				    size_t relnum,
				    Output_segment* tls_segment,
				    const elfcpp::Rel<32, false>& rel,
				    unsigned int r_type,
				    elfcpp::Elf_types<32>::Elf_Addr value,
				    unsigned char* view,
				    off_t view_size)
{
  // We have to actually change the instructions, which means that we
  // need to examine the opcodes to figure out which instruction we
  // are looking at.
  if (r_type == elfcpp::R_386_TLS_IE)
    {
      // movl %gs:XX,%eax  ==>  movl $YY,%eax
      // movl %gs:XX,%reg  ==>  movl $YY,%reg
      // addl %gs:XX,%reg  ==>  addl $YY,%reg
      tls::check_range(relinfo, relnum, rel.get_r_offset(), view_size, -1);
      tls::check_range(relinfo, relnum, rel.get_r_offset(), view_size, 4);

      unsigned char op1 = view[-1];
      if (op1 == 0xa1)
	{
	  // movl XX,%eax  ==>  movl $YY,%eax
	  view[-1] = 0xb8;
	}
      else
	{
	  tls::check_range(relinfo, relnum, rel.get_r_offset(), view_size, -2);

	  unsigned char op2 = view[-2];
	  if (op2 == 0x8b)
	    {
	      // movl XX,%reg  ==>  movl $YY,%reg
	      tls::check_tls(relinfo, relnum, rel.get_r_offset(),
                             (op1 & 0xc7) == 0x05);
	      view[-2] = 0xc7;
	      view[-1] = 0xc0 | ((op1 >> 3) & 7);
	    }
	  else if (op2 == 0x03)
	    {
	      // addl XX,%reg  ==>  addl $YY,%reg
	      tls::check_tls(relinfo, relnum, rel.get_r_offset(),
                             (op1 & 0xc7) == 0x05);
	      view[-2] = 0x81;
	      view[-1] = 0xc0 | ((op1 >> 3) & 7);
	    }
	  else
	    tls::check_tls(relinfo, relnum, rel.get_r_offset(), 0);
	}
    }
  else
    {
      // subl %gs:XX(%reg1),%reg2  ==>  subl $YY,%reg2
      // movl %gs:XX(%reg1),%reg2  ==>  movl $YY,%reg2
      // addl %gs:XX(%reg1),%reg2  ==>  addl $YY,$reg2
      tls::check_range(relinfo, relnum, rel.get_r_offset(), view_size, -2);
      tls::check_range(relinfo, relnum, rel.get_r_offset(), view_size, 4);

      unsigned char op1 = view[-1];
      unsigned char op2 = view[-2];
      tls::check_tls(relinfo, relnum, rel.get_r_offset(),
                     (op1 & 0xc0) == 0x80 && (op1 & 7) != 4);
      if (op2 == 0x8b)
	{
	  // movl %gs:XX(%reg1),%reg2  ==>  movl $YY,%reg2
	  view[-2] = 0xc7;
	  view[-1] = 0xc0 | ((op1 >> 3) & 7);
	}
      else if (op2 == 0x2b)
	{
	  // subl %gs:XX(%reg1),%reg2  ==>  subl $YY,%reg2
	  view[-2] = 0x81;
	  view[-1] = 0xe8 | ((op1 >> 3) & 7);
	}
      else if (op2 == 0x03)
	{
	  // addl %gs:XX(%reg1),%reg2  ==>  addl $YY,$reg2
	  view[-2] = 0x81;
	  view[-1] = 0xc0 | ((op1 >> 3) & 7);
	}
      else
	tls::check_tls(relinfo, relnum, rel.get_r_offset(), 0);
    }

  value = tls_segment->vaddr() + tls_segment->memsz() - value;
  if (r_type == elfcpp::R_386_TLS_IE || r_type == elfcpp::R_386_TLS_GOTIE)
    value = - value;

  Relocate_functions<32, false>::rel32(view, value);
}

// Relocate section data.

void
Target_i386::relocate_section(const Relocate_info<32, false>* relinfo,
			      unsigned int sh_type,
			      const unsigned char* prelocs,
			      size_t reloc_count,
			      unsigned char* view,
			      elfcpp::Elf_types<32>::Elf_Addr address,
			      off_t view_size)
{
  gold_assert(sh_type == elfcpp::SHT_REL);

  gold::relocate_section<32, false, Target_i386, elfcpp::SHT_REL,
			 Target_i386::Relocate>(
    relinfo,
    this,
    prelocs,
    reloc_count,
    view,
    address,
    view_size);
}

// Return the value to use for a dynamic which requires special
// treatment.  This is how we support equality comparisons of function
// pointers across shared library boundaries, as described in the
// processor specific ABI supplement.

uint64_t
Target_i386::do_dynsym_value(const Symbol* gsym) const
{
  gold_assert(gsym->is_from_dynobj() && gsym->has_plt_offset());
  return this->plt_section()->address() + gsym->plt_offset();
}

// Return a string used to fill a code section with nops to take up
// the specified length.

std::string
Target_i386::do_code_fill(off_t length)
{
  if (length >= 16)
    {
      // Build a jmp instruction to skip over the bytes.
      unsigned char jmp[5];
      jmp[0] = 0xe9;
      elfcpp::Swap_unaligned<32, false>::writeval(jmp + 1, length - 5);
      return (std::string(reinterpret_cast<char*>(&jmp[0]), 5)
              + std::string(length - 5, '\0'));
    }

  // Nop sequences of various lengths.
  const char nop1[1] = { 0x90 };                   // nop
  const char nop2[2] = { 0x66, 0x90 };             // xchg %ax %ax
  const char nop3[3] = { 0x8d, 0x76, 0x00 };       // leal 0(%esi),%esi
  const char nop4[4] = { 0x8d, 0x74, 0x26, 0x00};  // leal 0(%esi,1),%esi
  const char nop5[5] = { 0x90, 0x8d, 0x74, 0x26,   // nop
                         0x00 };                   // leal 0(%esi,1),%esi
  const char nop6[6] = { 0x8d, 0xb6, 0x00, 0x00,   // leal 0L(%esi),%esi
                         0x00, 0x00 };
  const char nop7[7] = { 0x8d, 0xb4, 0x26, 0x00,   // leal 0L(%esi,1),%esi
                         0x00, 0x00, 0x00 };
  const char nop8[8] = { 0x90, 0x8d, 0xb4, 0x26,   // nop
                         0x00, 0x00, 0x00, 0x00 }; // leal 0L(%esi,1),%esi
  const char nop9[9] = { 0x89, 0xf6, 0x8d, 0xbc,   // movl %esi,%esi
                         0x27, 0x00, 0x00, 0x00,   // leal 0L(%edi,1),%edi
                         0x00 };
  const char nop10[10] = { 0x8d, 0x76, 0x00, 0x8d, // leal 0(%esi),%esi
                           0xbc, 0x27, 0x00, 0x00, // leal 0L(%edi,1),%edi
                           0x00, 0x00 };
  const char nop11[11] = { 0x8d, 0x74, 0x26, 0x00, // leal 0(%esi,1),%esi
                           0x8d, 0xbc, 0x27, 0x00, // leal 0L(%edi,1),%edi
                           0x00, 0x00, 0x00 };
  const char nop12[12] = { 0x8d, 0xb6, 0x00, 0x00, // leal 0L(%esi),%esi
                           0x00, 0x00, 0x8d, 0xbf, // leal 0L(%edi),%edi
                           0x00, 0x00, 0x00, 0x00 };
  const char nop13[13] = { 0x8d, 0xb6, 0x00, 0x00, // leal 0L(%esi),%esi
                           0x00, 0x00, 0x8d, 0xbc, // leal 0L(%edi,1),%edi
                           0x27, 0x00, 0x00, 0x00,
                           0x00 };
  const char nop14[14] = { 0x8d, 0xb4, 0x26, 0x00, // leal 0L(%esi,1),%esi
                           0x00, 0x00, 0x00, 0x8d, // leal 0L(%edi,1),%edi
                           0xbc, 0x27, 0x00, 0x00,
                           0x00, 0x00 };
  const char nop15[15] = { 0xeb, 0x0d, 0x90, 0x90, // jmp .+15
                           0x90, 0x90, 0x90, 0x90, // nop,nop,nop,...
                           0x90, 0x90, 0x90, 0x90,
                           0x90, 0x90, 0x90 };

  const char* nops[16] = {
    NULL,
    nop1, nop2, nop3, nop4, nop5, nop6, nop7,
    nop8, nop9, nop10, nop11, nop12, nop13, nop14, nop15
  };

  return std::string(nops[length], length);
}

// The selector for i386 object files.

class Target_selector_i386 : public Target_selector
{
public:
  Target_selector_i386()
    : Target_selector(elfcpp::EM_386, 32, false)
  { }

  Target*
  recognize(int machine, int osabi, int abiversion);

 private:
  Target_i386* target_;
};

// Recognize an i386 object file when we already know that the machine
// number is EM_386.

Target*
Target_selector_i386::recognize(int, int, int)
{
  if (this->target_ == NULL)
    this->target_ = new Target_i386();
  return this->target_;
}

Target_selector_i386 target_selector_i386;

} // End anonymous namespace.
