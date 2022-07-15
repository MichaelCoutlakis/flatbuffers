## FlatBuffers MATLAB library code location
The code for the Flatbuffers library can be found at
`flatbuffers/include/flatbuffers`.

## Using the FlatBuffers Octave library

#### Install flatbuffers


### Make a schema
```flatbuffer
table MyBuffer
{
    m_Int:int32;
    m_VecFloat:[float];
    m_String:string;
}
```
### Compile the schema
```
cd C:\lib\vcpkg\buildtrees\flatbuffers\x86-windows-rel

flatc --cpp --gen-object-api -o OutputDirectory -I IncludeDirectory MyBuffer.fbs
```
Run the FlatOctave Octave script compiler on the Schema

Use the Octave script :smiley:

```Octave
 B.m_Int = 42;
 B.m_VecFloat = [1, 2, 3, 4];
 B.m_String = "Hello From Octave!";

 Bytes = MyBufferT_Pack(B);

 # Write a size prefix:
 Bytes = [WriteUint32(length(Bytes)), Bytes];

 # Save it to disk or send it over the network:
 fid = fopen("Filename");
 fwrite(fid, Bytes);
 fclose(fid);
```

Read the flatbuffer in C++
```cpp
    std::vector<uint8_t> Buffer;
    // ... Read data into Buffer
    MyBufferT MyBufferRx;
    flatbuffers::GetSizePrefixedRoot<MyBuffer>(Buffer.data())->UnpackTo(&MyBufferRx);
```

## Implemented Functionality
The following functionality has been implemented:

| Flatbuffer Type | Read Flatbuffer into Octave | Write Flatbuffer from Octave |
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