## FlatBuffers MATLAB library code location
The code for the Flatbuffers library can be found at
`flatbuffers/include/flatbuffers`.

## Using the FlatBuffers MATLAB library

FlatBuffers supports both reading and writing FlatBuffers in MATLAB.
See [the section on implemented functionality](#implemented-functionality)
for an overview of which flatbuffers features are currently implemented.

To use FlatBuffers in your code, first generate the MATLAB functions to read and write
objects defined by your schema with the `--matlab` option to `flatc`. Then you can 
include both FlatBuffers and the generated code to read or write FlatBuffers.

### Write a schema

In file MyBuffers.fbs
```flatbuffer
table NestedTable
{
  m_Int:int32;
}

table MyTable
{
    m_Int:int32;
    m_VecFloat:[float];
    m_String:string;
    m_NestedTable:NestedTable;
}

```

### Compile the schema
```
flatc --matlab -o OutputDirectory -I IncludeDirectory MyBuffers.fbs
```

This will give you a ```MyBuffers_generated.m``` script file. You will need this and the ```flatbuffers.m``` file accessible from MATLAB for the following steps.

### Write a Flatbuffers file from MATLAB
```MATLAB
  % Create a MyTable object
  MyTable.m_Int = 42;
  MyTable.m_VecFloat = [1, 2, 3, 4, 5];
  MyTable.m_String = "Hello MATLAB World!";
  MyTable.m_NestedTable.m_Int = 77;
  
  % Write the object to a byte array:
  BytesTx = MyBuffers_generated.MyTableT_Pack(MyTable);

  % Add a size prefix and write the buffer to disk:
  flatbuffers.AddSizePrefixAndWrite(BytesTx, "MyTable.fb");
```

### Read a Flatbuffers file into MATLAB
```MATLAB
  % Assuming the file "MyTable.fb" exists, e.g. written by the above example or saved by another language, e.g. a c++ program.
  % Read the bytes:
  BytesRx = flatbuffers.ReadBufferFromDisk("MyTable.fb");
  % Parse the bytes to get a "MyTable" object. Note we start reading at position 5 due to the 4
  % byte size prefix
  MyTable = MyBuffers_generated.MyTableT_Unpack(BytesRx, 5);
```
You should now have a ```MyTable``` object like this:
```MATLAB
MyTable = 

  struct with fields:

            m_Int: 42
       m_VecFloat: [5×1 single] % [1; 2; 3; 4; 5]
         m_String: 'asdf'
    m_NestedTable: [1×1 struct] % struct with fields: m_Int: 77

```


## Implemented Functionality
The following functionality has been implemented:

| Flatbuffer Type | Read Flatbuffer into MATLAB | Write Flatbuffer from MATLAB |
|---|:---:|:---:|
|int| :heavy_check_mark: | :heavy_check_mark: |
|float| :heavy_check_mark: | :heavy_check_mark: |
|string| :heavy_check_mark: | :heavy_check_mark: |
|enum | :x: | :x: |
|:[float] (vector&lt;float&gt;)| :heavy_check_mark: | :heavy_check_mark: |
|:[string] (vector&lt;string&gt;)| :heavy_check_mark: | :heavy_check_mark: |
|Table| :heavy_check_mark: | :heavy_check_mark:|
|:[Table] (vector&lt;Table&gt;)| :heavy_check_mark: | :heavy_check_mark: |
|Array| :x: | :x: |
|Default Values | :x: | :x:|
|Optional Values | :x: | :x:|
|Required Values | :x: | :x:|
|Namespace |:x:| :x: |