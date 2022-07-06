/*
 * Copyright 2021 Michael Coutlakis. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// independent from idl_parser, since this code is not needed for most clients

/*
Generate Octave script from the idl / schema:
*/

#include <unordered_set>
#include <iostream>

#include "flatbuffers/code_generators.h"
#include "flatbuffers/flatbuffers.h"
#include "flatbuffers/flatc.h"
#include "flatbuffers/idl.h"
#include "flatbuffers/util.h"

namespace flatbuffers
{
namespace octave
{
const std::string nl = "\n";
const std::string tb = "\t";
const std::string BufOutline = "BufOutline";
inline std::string tabs(size_t N)
{
  std::string s;
  for(size_t n = 0U; n != N; ++n)
    s += tb;
  return s;
}
// Extension of IDLOptions for cpp-generator:
struct IDLOptionsOctave : public IDLOptions {
  // All fields start with 'oct_' prefix to distuinguish from the base options
  IDLOptionsOctave(const IDLOptions& opts) : IDLOptions(opts) {}
};

class OctaveGenerator : public BaseGenerator {
public:
  OctaveGenerator(const Parser& parser, const std::string& path,
    const std::string& file_name, IDLOptionsOctave opts)
    : BaseGenerator(parser, path, file_name, "", "", "m"), opts_(opts)
  {
    static const char* const keywords[] = {
      "case",
      "do",
      "for",
      "function"
      "if",
      "switch",
      "while",
      nullptr,
    };
    for(auto kw = keywords; *kw; kw++) keywords_.insert(*kw);
  }
  // Iterate through all the definitions we haven't generated code for and
  // output them to a single file.
  bool generate() override
  {
//code_.Clear();
    code_.clear();
    code_ += "% " + std::string(FlatBuffersGeneratedWarning()) + nl;

    // We will probably write it to a octave script file, start with empty statement:
    code_ += "1;";
    code_ += nl;

    // Is it possible for forward declarations and circular references in
    // Octave?

    // Generate code for all structs, then all tables.
    // for (auto it = parser_.structs_.vec.begin();
    //     it != parser_.structs_.vec.end(); ++it) {
    //  const auto &struct_def = **it;
    //  if (struct_def.fixed && !struct_def.generated) {
    //    SetNameSpace(struct_def.defined_namespace);
    //    GenStruct(struct_def);
    //  }
    //}
    for(auto it = parser_.structs_.vec.begin();
      it != parser_.structs_.vec.end(); ++it)
    {
      const auto& struct_def = **it;
      if(!struct_def.fixed && !struct_def.generated)
      {
        SetNameSpace(struct_def.defined_namespace);
        GenTable(struct_def);
      }
    }

    const auto file_path = GeneratedFileName(path_, file_name_, opts_);
    //const auto final_code = code_.ToString();

    // Save the file:
    return SaveFile(file_path.c_str(), code_, false);
  }

private:
 //CodeWriter code_;
  std::string code_;

  std::unordered_set<std::string> keywords_;

  const IDLOptionsOctave opts_;

  // Escape the name if necessary so it doesn't match any of the languages special keywords
  std::string EscapeKeyword(const std::string& name) const
  {
    return keywords_.find(name) == keywords_.end() ? name : name + "_";
  }

  // Find a suitable name for the type definition
  std::string Name(const Definition& def) const
  {
    return EscapeKeyword(def.name);
  }

  // Wrap the string in quotation marks
  std::string Quote(const std::string& str) { return "\"" + str + "\""; }

  static std::string NativeName(const std::string& name, const StructDef* sd,
    const IDLOptions& opts)
  {
    return sd && !sd->fixed ? opts.object_prefix + name + opts.object_suffix
      : name;
  }

  std::string GenFieldOffsetName(const FieldDef& field)
  {
    std::string uname = Name(field);
    std::transform(uname.begin(), uname.end(), uname.begin(), [](char c) {return static_cast<char>(std::toupper(c));});
    return "VT_" + uname;
  }

  // Generate .m to read the specified number of bytes out of b.
  std::string GenReadBytesAtIndex(const std::string& idx, unsigned num_bytes)
  {
    if(num_bytes == 0U)
      return "[]";
    return "b(" + idx + ":" + idx + " + " + NumToString(num_bytes - 1U) + ")";
  }
  // Generate .m for a function that reads a uint32 from the specified index:
  std::string GenReadUint32(const std::string& idx)
  {
    // Note: ReadUint32 is a function that will be implemented in the Octave flatbuffers library.
    // We extract the 4 bytes before passing to the function as Octave doesn't have pass by pointer
    // and I don't want to be passing a potentially large b around the whole time
    return "ReadUint32(" + GenReadBytesAtIndex(idx, 4U) + ")";
  }
  // Generate .m for a function that reads an int32 from the specified index
  std::string GenReadInt32(const std::string& idx)
  {
    return "ReadInt32(" + GenReadBytesAtIndex(idx, 4U) + ")";
  }
  // Generate .m for a function that reads a uint16 from the specified index
  std::string GenReadUint16(const std::string& idx)
  {
    return "ReadUint16(" + GenReadBytesAtIndex(idx, 4U) + ")";
  }

  // Generate .m for something like typecast(b(idx:idx + idx_add), type_to)
  std::string GenTypecast(const std::string& idx, const std::string& idx_add, const std::string& type_to)
  {
    return "typecast(b(" + idx + ":" + idx + " + " + idx_add + ")," + Quote(type_to) + ")";
  }

  std::string GenIdxFieldNameOff(const std::string& field_name) { return "idx_" + field_name + "_off"; }
  std::string GenIdxFieldName(const std::string& field_name) { return "idx_" + field_name; }
  std::string GenLenFieldName(const std::string& field_name) { return "len_" + field_name; }
  std::string GenUnpackFunctionName(const std::string& type_name) { return type_name + "_Unpack"; }

  std::string GenPackFunctionName(const std::string& type_name) { return type_name + "_Pack"; }

  // Generate a member, including default values
  void GenMember(const FieldDef& field, const std::string& parent)
  {
    if(!field.deprecated &&  // Deprecated fields not accessible
      field.value.type.base_type != BASE_TYPE_UTYPE &&    // What are these actually doing?
      field.value.type.base_type != BASE_TYPE_UTYPE &&
      (field.value.type.base_type != BASE_TYPE_VECTOR ||
        field.value.type.element != BASE_TYPE_UTYPE))
    {
// In Octave, it's dynamically typed
      std::string member_def = parent + "." + Name(field) + " = [];";
      code_ += member_def;
    }

  }
  // Generate the declaration of the table. We don't really need this for Octave
  // as it can be constructed dynamically
  void GenNativeTable(const StructDef& struct_def)
  {
    const auto native_name = NativeName(Name(struct_def), &struct_def, opts_);
  }

  std::string GetOctaveType(const BaseType& type)
  {
    std::map<flatbuffers::BaseType, std::string> mapCppOctave = {
      /* This is following the ordering of types listed under "cast" in Octave's pdf manual. Also see FLATBUFFERS_GEN_TYPES_SCALAR */
      {BASE_TYPE_DOUBLE, "double"},
      {BASE_TYPE_FLOAT, "single"},
      {BASE_TYPE_BOOL, "logical"},
      {BASE_TYPE_CHAR, "char"},
      //{BASE_TYPE_, "int8"},
      {BASE_TYPE_SHORT, "int16"},
      {BASE_TYPE_INT, "int32"},
      {BASE_TYPE_LONG, "int64"},
      {BASE_TYPE_UCHAR, "uint8"},
      {BASE_TYPE_UTYPE, "uint8"},
      {BASE_TYPE_USHORT, "uint16"},
      {BASE_TYPE_UINT, "uint32"},
      {BASE_TYPE_ULONG, "uint64"},
    };
    std::string octave_type = mapCppOctave[type];
    return octave_type;
  }

  // Return an unpack string for the basic type
  std::string GenUnpackStringBasicField(const std::string& field_name, const std::string& struct_field, const Type& type)
  {
    std::string OctaveType = GetOctaveType(type.base_type);

    if(OctaveType.empty())
      std::cout << "Warning: Could not find base type for " << type.base_type << " field name " << field_name << std::endl;
    // Note: The index has already been named as idx_Name_off in GenTable. For inline types it's an absolute index, but generate the same name (with an _off) to match GenTable
    std::string idxField = GenIdxFieldNameOff(field_name);
    std::string unpack = struct_field + " = typecast(b(" + idxField + ":" + idxField + " + " + NumToString(SizeOf(type.base_type) - 1) + "), " + Quote(OctaveType) + ")";
    return unpack;
  }
  /* Return a pack string for a basic type. */
  std::string GenPackStringBasicField(const std::string& field_name, const std::string& struct_field, const Type& type)
  {
    std::ignore = struct_field;
    std::string OctaveType = GetOctaveType(type.base_type);

    if(OctaveType.empty())
      std::cout << "Warning: Could not find base type for " << type.base_type << " field name " << field_name << std::endl;

    std::string str;
    str += "BufInline = [BufInline, typecast(" + OctaveType + "(" + "T." + field_name + "), \"uint8\")]" + nl;
    return str;
  }

  std::string GenUnpackVector(const std::string& field_name, const std::string& struct_field, const Type& type)
  {
    std::string unpack;
    auto vector_type = type.VectorType();
    std::string idxFieldNameOff = GenIdxFieldNameOff(field_name);
    std::string idxField = GenIdxFieldName(field_name);
    std::string idxVecLen = GenLenFieldName(field_name);
    std::string struct_name = type.struct_def ? NativeName(Name(*type.struct_def), type.struct_def, opts_) : "";

    unpack += "offOuter = " + GenReadUint32(idxFieldNameOff) + nl;
    unpack += idxField + " = offOuter + " + idxFieldNameOff + nl;
    unpack += idxVecLen + " = " + GenReadUint32(idxField) + nl;


    bool bGenForLoop = vector_type.base_type == BASE_TYPE_STRING || vector_type.base_type == BASE_TYPE_STRUCT;
    if(bGenForLoop)
    {
      unpack += "for(uK = 1:" + idxVecLen + ")" + nl;
      unpack += "idxElemOffPos = " + idxField + " + 4 * uK" + nl;
    }

    //unpack += struct_field + "(uK)" + " = " + GenUnpackFunctionName(struct_name) + "(b, " + "idxElemOffPos" + ")" + nl;


    switch(vector_type.base_type)
    {
    case BASE_TYPE_STRING:
    {
// For a string there is a uint32_t length followed by the string bytes:
      unpack += "idxElemK = idxElemOffPos + " + GenReadUint32("idxElemOffPos") + nl;
      unpack += "lenString = " + GenReadUint32("idxElemK") + nl;
      unpack += struct_field + "{uK} = cast(b(idxElemK  + 4:idxElemK + 4 + lenString - 1), \"char\")'" + nl;
      break;
    }
    case BASE_TYPE_STRUCT:
    {
//std::string struct_name = NativeName(Name(*type.struct_def), type.struct_def, opts_);
//unpack += "for(uK = 1:" + idxVecLen + ")" + nl;
//unpack += "idxElemOffPos = " + idxField + " + 4 * uK" + nl;
      unpack += struct_field + "(uK)" + " = " + GenUnpackFunctionName(struct_name) + "(b, " + "idxElemOffPos" + ")" + nl;
      //unpack += "end" + nl;
      break;
    }
    default:
    {
//unpack += GenFieldOffsetName(field_name) + " = idxRT"
//std::string idxFieldNameOff = GenIdxFieldNameOff(field_name);
//std::string idxField = GenIdxFieldName(field_name);
//std::string idxVecLen = GenLenFieldName(field_name);
      std::string octave_type = GetOctaveType(type.element);
      if(octave_type.empty())
        std::cout << "Warning: Could not find octave type for " << field_name << std::endl;

      //unpack += "offOuter = " + GenReadUint32(idxFieldNameOff) + nl;
      //unpack += idxField + " = offOuter + " + idxFieldNameOff + nl;
      //unpack += idxVecLen + " = " + GenReadUint32(idxField) + nl;
      unpack += struct_field + " = " + GenTypecast(idxField + " + 4", idxVecLen + "*4 - 1", octave_type) + nl;
      break;
    }
    }
    if(bGenForLoop)
      unpack += "end" + nl;
    return unpack;
  }

  std::string GenPackVector(const std::string& field_name, const std::string& struct_field, const Type& type)
  {
    std::ignore = struct_field;
    std::string pack;
    auto vector_type = type.VectorType();
    pack += GenPackOutline();

    bool bGenForLoop = vector_type.base_type == BASE_TYPE_STRING || vector_type.base_type == BASE_TYPE_STRUCT;

    // For tables and strings, we will need to loop through the elents and write each one as a vector element
    // in the outline data
    if(bGenForLoop)
    {
      // First we need an offset to where the vector is in outline data:
      pack += "N_" + field_name + " = length(" + struct_field + ");" + nl;
      // Write the vector size:
      pack += "BufOutline = [BufOutline, WriteUint32(N_" + field_name + ")];" + nl;
      // Now, each element in this vector is an offset to where the element (table, string) is in the outline
      // data. For ease, we will make another outline buffer:
      pack += "VecOffsets_" + field_name + " = [];" + nl;
      pack += "VecOutline_" + field_name + " = [];" + nl;
      
    }
    switch(vector_type.base_type)
    {
    case BASE_TYPE_STRUCT:
    {
      std::string struct_name = NativeName(Name(*type.struct_def), type.struct_def, opts_);
      pack += "for k = 1:length(" + struct_field + ")" + nl;
      pack += "offTo_" + field_name + "_k = N_" + field_name + " * 4 + 4 - 4 * k + length(VecOutline_" + field_name + ");" + nl;

      std::string BytesFieldName = "Bytes_" + field_name + "_k";
      pack += BytesFieldName + " = " + GenPackFunctionName(struct_name) + "(" + struct_field + "(k));" + nl;
      // The first uint will be the RTO: Erase it as the offset will go inside the parent offset:
      pack += "RTO" + field_name + " = ReadUint32(" + BytesFieldName + "(1:4));" + nl;
      // The offset to the element will now be the vector offset + the root table offset - the erased original root table offset index.
      pack += "VecOffsets_" + field_name + " = [VecOffsets_" + field_name + ", WriteUint32(offTo_" + field_name + "_k + RTO" + field_name + " - 4)];" + nl;
      //pack += "VecOutline_" + field_name + " = [VecOutline_" + field_name + ", " + GenPackFunctionName(struct_name) + "(" + struct_field + "(k))];" + nl;
      pack += "VecOutline_" + field_name + " = [VecOutline_" + field_name + ", " + BytesFieldName + "(5:end)];" + nl;
      break;
    }
    case BASE_TYPE_STRING:
    {
      pack += "for k = 1:length(" + struct_field + ")" + nl;
      pack += "str_" + field_name + "_k = " + struct_field + "{k};" + nl;
      pack += "offTo_" + field_name + "_k = N_" + field_name + " * 4 + 4 - 4 * k + length(VecOutline_" + field_name + ");" + nl;
      pack += "VecOffsets_" + field_name + " = [VecOffsets_" + field_name + ", WriteUint32(offTo_" + field_name + "_k)];" + nl;
      pack += "VecOutline_" + field_name + " = [VecOutline_" + field_name + ", WriteString(str_" + field_name + "_k)];" + nl;
      break;
    }
    default:
    {
      // Works for vectors of plain Octave types
      std::string octave_type = GetOctaveType(type.element);
      if(octave_type.empty())
        std::cout << "Warning: Could not find octave type for " << field_name << std::endl;

      // Pack the vector number of elements:
      pack += "Bytes = WriteUint32(length(T." + field_name + "));" + nl;
      // Pack the vector bytes
      pack += "Bytes = [Bytes, typecast(" + octave_type + "(T." + field_name + "), \"uint8\")(:)'];" + nl;
      pack += BufOutline + " = [" + BufOutline + ", Bytes];" + nl;
      break;
    }
    }
    if(bGenForLoop)
    {
      pack += "end" + nl;
      pack += "BufOutline = [BufOutline, VecOffsets_" + field_name + ", VecOutline_" + field_name + "];" + nl;
    }
    return pack;
  }

  std::string GenUnpackStruct(const std::string& field_name, const std::string& struct_field, const Type& type)
  {
    std::string struct_name = NativeName(Name(*type.struct_def), type.struct_def, opts_);
    std::string unpack;
    unpack += struct_field + " = " + GenUnpackFunctionName(struct_name) + "(b, " + GenIdxFieldNameOff(field_name) + ")";
    return unpack;
  }
  // Use this when data is going to be added to the ouline buffer.
  [[nodiscard]] std::string GenPackOutline()
  {
    std::string pack;
    /* The offset from the table to the outline data: */
    pack += "offsOutline(end + 1) = length(BufOutline) - length(BufInline)" + nl;
    /* Allocate an extra index for the offset as we will have to fill this in with the actual offset */
    pack += "idxOffsOutline(end + 1) = length(BufInline) + 1" + nl;
    /* Reserve space where the offset to the vector will be placed in the table (inline data): */
    pack += "BufInline = [BufInline, uint8([0, 0, 0, 0])];" + nl;
    /* Now the specific outline data can be written: */
    return pack;
  }
  // This is called when we're packing a table which is a member of another table.
  std::string GenPackStruct(const std::string& field_name, const std::string& struct_field, const Type& type)
  {
    std::ignore = struct_field;
    std::ignore = type;
    std::string struct_name = NativeName(Name(*type.struct_def), type.struct_def, opts_);
    std::string pack;

    //TODO what if the struct is empty?

    pack += GenPackOutline();
    std::string BytesFieldName = "Bytes" + field_name;
    pack += BytesFieldName + " = " + GenPackFunctionName(struct_name) + "(T." + field_name + ")" + nl;
    // The first uint will be the RTO: Erase it as the offset will go inside the parent offset:
    pack += "RTO" + field_name + " = ReadUint32(" + BytesFieldName + "(1:4));" + nl;
    //pack += BytesFieldName + " = " + BytesFieldName + "(5:end);" + nl;
    pack += "offsOutline(end) += RTO" + field_name + " - 4;" + nl;
    pack += "BufOutline = [BufOutline, " + BytesFieldName + "(5:end)];" + nl;
    return pack;
  }
  // Return an unpack string for the type
  // field name is the member we are unpacking
  // struct_field is the output field of the struct we are unpackint to, e.g. MsgRx.m_member.
  // The idx_FieldName_off has already been generated
  // So what this function generates is something like:
  // ... unpacking code...
  // MsgRx.m_member = ... unpacking code ...
  std::string GenUnpackStringForField(const std::string& field_name, const std::string& struct_field, const Type& type)
  {
    if(IsScalar(type.base_type))
      return GenUnpackStringBasicField(field_name, struct_field, type);
    else if(IsArray(type))
    {
      auto element_type = type.VectorType();
    }
    else if(IsVector(type))
    {
      return GenUnpackVector(field_name, struct_field, type);
    }
    else if(type.base_type == BASE_TYPE_STRING)
    {
// index of the string member offset in the table:
      std::string unpack;
      std::string idxFieldNameOff = GenIdxFieldNameOff(field_name);
      std::string idxField = GenIdxFieldName(field_name);
      unpack += "offOuter = " + GenReadUint32(idxFieldNameOff) + nl;
      unpack += idxField + " = " + idxFieldNameOff + " + offOuter" + nl;
      unpack += GenLenFieldName(field_name) + " = " + GenReadUint32(idxField) + nl;
      unpack += struct_field + " = cast(b(" + idxField + " + 4:" + idxField + " + 4 + " + GenLenFieldName(field_name) + " - 1), \"char\")'" + nl;
      return unpack;
    }
    else if(type.base_type == BASE_TYPE_VECTOR)
    {

    }
    else if(type.base_type == BASE_TYPE_STRUCT)
    {
      return GenUnpackStruct(field_name, struct_field, type);
    }
    else
    {
// TODO
    }
    return "warning(\"Unhandled GenUnpackStringBasicField\")" + nl;
  }
  std::string GenPackStringForField(const std::string& field_name, const std::string& struct_field, const Type& type)
  {
    if(IsScalar(type.base_type))
      return GenPackStringBasicField(field_name, struct_field, type);
    else if(IsVector(type))
    {
      return GenPackVector(field_name, struct_field, type);
    }
    else if(type.base_type == BASE_TYPE_STRING)
    {
      std::string str;
      str += GenPackOutline();
      str += "BufOutline = [BufOutline, WriteString(T." + field_name + ")]" + nl;
      return str;
    }
    else if(type.base_type == BASE_TYPE_STRUCT)
    {
      return GenPackStruct(field_name, struct_field, type);
    }
    //else if (type.base_type == BASE_TYPE)
    return "Warning: \"Unhandled GenPackStringForField for field " + field_name + "\"";
  }
  std::string GenOctaveVT(const StructDef& struct_def)
  {
    std::string vtable;
    // Generate the vtable field ID constants
    // We're going to form an Octave structure called VT with an array for the offsets and a cell array for the names,
    // e.g. VT = struct("Offsets", [4, 12, 20], "Fields", {{"m_Name1"; "m_Name2"; "m_Name3"}});
    if(struct_def.fields.vec.size() > 0)
    {
      std::string offsets;
      std::string field_names;

      for(auto it = struct_def.fields.vec.begin();
        it != struct_def.fields.vec.end(); ++it)
      {
        const auto& field = **it;

        if(!field.deprecated)
        {
          offsets += NumToString(field.value.offset) + ", ";
          field_names += "\"" + field.name + "\";";
        }
      }
      vtable = "VT = struct(\"Offsets\", [" + offsets + "], \"Fields\", {{" + field_names + "}});" + nl;
    }
    return vtable;
  }
  void GenTable(const StructDef& struct_def)
  {
    GenNativeTable(struct_def);

    std::string struct_name = NativeName(Name(struct_def), &struct_def, opts_);
    code_ += "function " + struct_name + " = " + GenUnpackFunctionName(struct_name) + "(b, idxBuf)" + nl;

    // Declare struct (empty)
    code_ += struct_name + " = {};" + nl;
    /* Offset to the root table: */
    code_ += "offRT = " + GenReadUint32("idxBuf") + nl;

    /* Root table offset in Octave's index into the bytes: */
    code_ += "idxRT = offRT + idxBuf" + nl;

    /* Offset to the VT from the RT: */
    code_ += "offVT = " + GenReadInt32("idxRT") + nl;
    /* Start of the vtable in Octave's index: */
    code_ += "idxVT = int32(idxRT) - offVT" + nl;
    /* Size of the VT: */
    code_ += "sizeVT = " + GenReadUint16("idxVT") + nl;
    /* Determine the inline data size and hence the number of fields: */
    //code_ += "sizeInline = " + GenReadUint16("idxVT + 2") + nl;
    code_ += "N = int32(sizeVT / 2 - 2)" + nl;

    code_ += "FieldOffsets = typecast(b(idxVT + 4:idxVT + 4 + 2*N - 1), \"uint16\")" + nl;
    // Make sure b is bytes:

    // Generate the vtable field ID constants
    // We're going to form an Octave structure called VT with an array for the offsets and a cell array for the names,
    // e.g. VT = struct("Offsets", [4, 12, 20], "Fields", {{"m_Name1"; "m_Name2"; "m_Name3"}});
    if(struct_def.fields.vec.size() > 0)
    {
      std::string vtable;
      std::string offsets;
      std::string field_names;

      for(auto it = struct_def.fields.vec.begin();
        it != struct_def.fields.vec.end(); ++it)
      {
        const auto& field = **it;

        if(!field.deprecated)
        {
          offsets += NumToString(field.value.offset) + ", ";
          field_names += "\"" + field.name + "\";";
        }
      }
      vtable = "VT = struct(\"Offsets\", [" + offsets + "], \"Fields\", {{" + field_names + "}});" + nl;
      code_ += vtable;
    }

    // Now we need to generate unpack code for each member depending on its type:
    size_t K = 1U;    // Note Octave 1 based
    for(auto it = struct_def.fields.vec.begin(); it != struct_def.fields.vec.end(); ++it)
    {
      const auto& field = **it;

      if(!field.deprecated)
      {
        const bool is_struct = IsStruct(field.value.type);
        const bool is_scalar = IsScalar(field.value.type.base_type);

        code_ += "if(N >= " + NumToString(K) + " && FieldOffsets(" + NumToString(K) + ") ~= 0)" + nl;
         //code_ += "if(FieldOffsets(" + NumToString(K) + ") ~= 0)" + nl;
        //code_ += GenTypeGet()
        // idx_FieldName_off, index of the field name offset in the root table
        code_ += GenIdxFieldNameOff(field.name) + " = idxRT + uint32(FieldOffsets(" + NumToString(K) + "))" + nl;
        // Generate code for where to put the field in the struct that we are unpacking to:
        std::string struct_field = struct_name + ".(VT.Fields{" + NumToString(K) + "})";
        code_ += GenUnpackStringForField(field.name, struct_field, field.value.type) + nl;

        // In Octave it's important that all the structs in a vector of structs have the same fields,
        // otherwise there will be errors assigning structs with incompatible fields... .
        // So if the field is empty put an empty field there
        if(!is_scalar)
        {
          code_ += "else" + nl;
          code_ += struct_field + " = {}" + nl;
        }
        else
        {
          code_ += "else" + nl;
          code_ += struct_field + " = 0;" + nl; // When unpacking, if scalar fields were not packed they get the default value
        }
        code_ += "endif" + nl;
      }
      ++K;
    }

    code_ += std::string("endfunction") + nl;


    // Now we can generate code to pack a table:
    GenPackTable(struct_def);
    /* For a start let's ignore VT repetition.
    * Each table will have a VT, followed by the table inline data and then the outline data.
    */
  }
  void GenPackTable(const StructDef& struct_def)
  {
    std::string str;
    std::string struct_name = NativeName(Name(struct_def), &struct_def, opts_);
    /* Octave function to  unpack the table T. Return packed bytes B. */
    str += "function B = " + GenPackFunctionName(struct_name) + "(T)" + nl;
    /* Generate a V Table: */
    str += GenOctaveVT(struct_def);
    /* We need to go through each member and do three things:
    * 1) Store an offset for the flatbuffer vtable which will be constructed in the future
    * 2) Write any inline data into an inline buffer and record inline length
    * 3) Write any outline data into an outline buffer, storing indexes to the locations in the inline buffer which points to the outline data
    */
    /*
    * Step 1:
    */
    /* Number of members */
    str += "N = length(VT.Offsets);" + nl;
    /* Byte length of the VT. */
    str += "lenVT = 4 + N * 2" + nl;
    /* Length of inline data, initialized with the length of the VTO. */
    //str += "lenInline = 4;" + nl;

    /* Buffer for inline data, reserving space for the VTO: */
    str += "BufInline = WriteInt32(0);" + nl;
    /* VT offsets to each member: */
    str += "offsVT = zeros(1, N);" + nl;
    /* Indexes into the table to offsets to out of line data. We need to come back here
    later once all the inline data has been added to fill these in: */
    str += "idxOffsOutline = [];" + nl;
    /* Offsets to the outline data for the corresponding entry in idxOffsOutline: */
    str += "offsOutline = [];" + nl;
    /* Buffer for outline data: */
    str += "BufOutline = [];" + nl;
    /* Now we generate pack code for each member depending on its type: */
    size_t K = 1U;    // Note Octave 1 based
    for(auto it = struct_def.fields.vec.begin(); it != struct_def.fields.vec.end(); ++it)
    {
      const auto& field = **it;

      if(!field.deprecated)
      {
        const bool is_struct = IsStruct(field.value.type);
        const bool is_scalar = IsScalar(field.value.type.base_type);
        std::string struct_field = "T.(VT.Fields{" + NumToString(K) + "})";

        
        //str += "if(isfield(T, VT.Fields{" + NumToString(K) + "})" + nl;

        str += "if(isfield(T, VT.Fields{" + NumToString(K) + "}) && !isempty(" + struct_field + "))" + nl;

        /* What offset to write into the VT later: */
        str += "offsVT(" + NumToString(K) + ") = length(BufInline);" + nl;
        //std::string struct_field = struct_name + ".(VT.Fields{" + NumToString(K) + "})";
        
        str += GenPackStringForField(field.name, struct_field, field.value.type);
        str += "endif" + nl;
        //str += "endif" + nl;
      }
      ++K;
    }
    /* Fill in the table offsets to outline data: */
    str += "if(length(idxOffsOutline) ~= length(offsOutline))" + nl;
    str += "error(\"Mismatched offset lengths for field" + struct_name + "\");" + nl;
    str += "end" + nl;
    str += "for(k = 1:length(idxOffsOutline))" + nl;
    str += "idx = idxOffsOutline(k);" + nl;
    str += "BufInline(idx:idx + 3) = WriteUint32(offsOutline(k) + length(BufInline));" + nl;
    str += "end" + nl;

    /* Write the VT: */
    str += "BufVT(1:2) = WriteUint16(lenVT);" + nl;
    //str += "BufVT(3:4) = WriteUint16(4 + lenInline);" + nl;
    str += "BufVT(3:4) = WriteUint16(length(BufInline));" + nl;
    str += "for(idxElem = 1:N)" + nl;
    str += "BufVT = [BufVT, WriteUint16(offsVT(idxElem))]" + nl;
    str += "end" + nl;

    /* Write the VTO: We're not treating re-useable VT here. Just offset it from the RT by the VT length */
    str += "BufInline(1:4) = WriteInt32(length(BufVT));" + nl;
    /* Write the size prefix and RTO: */
    str += "lenInner = length(BufInline) + length(BufVT)" + nl;
    str += "lenPad = 0;" + nl;
    str += "if((r = rem(lenInner, 4)) ~= 0)" + nl;
    str += "lenPad = 4 - r;" + nl;
    str += "end" + nl;
    str += "offRT = 4 + lenPad + length(BufVT)" + nl;
    str += "B = [WriteUint32(offRT), zeros(1, lenPad), BufVT, BufInline, BufOutline]" + nl;
    /* Write the outline data */

    /* Combine the VT, inline and outline data, set the offsets */
    //str += "B = [BufInline, BufOutline]" + nl;
    str += "endfunction" + nl;
    code_ += str;
  }
  // Set up the correct namespace. Only open / close what is necessary.
  void SetNameSpace(const Namespace* ns)
  {
// Do nothing for now.
    std::ignore = ns;
  }
};
}  // namespace octave
bool GenerateOctave(const Parser& parser, const std::string& path,
  const std::string& file_name)
{
  octave::IDLOptionsOctave opts(parser.opts);

  octave::OctaveGenerator generator(parser, path, file_name, opts);
  return generator.generate();
}
}  // namespace flatbuffers
