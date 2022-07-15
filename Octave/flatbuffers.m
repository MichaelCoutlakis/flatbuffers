%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% \brief  Script file for Octave flatbuffers. If you like this is the Octave
%         side of the flatbuffers library, providing any useful functions
% \author Michael Coutlakis
% \date   2021-03-06
% \note:  Since this is a script file, before using it load the functions with
%         "flatbuffers"
% \note:  See https://google.github.io/flatbuffers/flatbuffers_internals.html
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
classdef flatbuffers
	methods(Static)

		% Read a buffer off disk:
		function Bytes = ReadBufferFromDisk(filename)
		  fid = fopen(filename);
		  fseek(fid, 0, SEEK_END);
		  FileLength = ftell(fid);
		  fseek(fid, 0, SEEK_SET);
		  Bytes = uint8(fread(fid, FileLength));
		endfunction

		% Add a size prefix to the bytes and then write to disk
		function AddSizePrefixAndWrite(Bytes, filename)
		  % Write a size prefix:
		  Bytes = [WriteUint32(length(Bytes)), Bytes]

		  % Save to file:
		  fid = fopen(filename, "wb");
		  fwrite(fid, Bytes);
		  fclose(fid);
		endfunction

		% Read an inline field, returning the default value if it's not there:
		function R = FlatBuffersReadScalar(bytes, DefaultVal)
		  x = 2  
		endfunction

		function Fun2()
		  x = 3
		endfunction

		% Read a uint32 from the 4 bytes
		function u = ReadUint32(b)
		  u = typecast(b(1:4), "uint32");
		endfunction

		% Read an int32 from the 4 bytes
		function u = ReadInt32(b)
		  u = typecast(b(1:4), "int32");
		endfunction

		% Read a uint16 from the 2 bytes
		function u = ReadUint16(b)
		  u = typecast(b(1:2), "uint16");
		endfunction

		% Write a uint16:
		function B = WriteUint16(u)
		  B = typecast(uint16(u), "uint8");
		endfunction

		% Write a uint32
		function B = WriteUint32(u)
		  B = typecast(uint32(u), "uint8");
		endfunction

		% Write and int
		function B = WriteInt32(i32)
		  B = typecast(int32(i32), "uint8");
		endfunction

		% Write a string:
		function B = WriteString(str)
		  bCol = iscolumn(str);
		  if(bCol)
			str = str';
		  end
		  % First 4 bytes is the length:
		  B = WriteUint32(length(str));
		  % Now the string characters: There's always a null padding at the end:
		  B = [B, uint8(str), uint8(0)];
		  % Make sure the total length is a multiple of 4, if not zero pad:
		  if((r = rem(length(B), 4)) ~= 0)
			B = [B, uint8(zeros(1, 4 - r))];
		  end
		  if(bCol)
			B = B';
		  end
		endfunction

		% Write a vtable to a buffer:
		function Bytes = WriteVT(VT)
		  Bytes = uint8(0);
		endfunction
	end
end