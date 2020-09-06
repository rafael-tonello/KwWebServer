# JsonMaker (A self contained and easy to use JSON library)

JsonMaker is a self-contained library to make working with JSON easier.

Each instance of this library represents a JSON object. This instance can do many things, such as parsing a string with json, exporting its data to a string, and allowing you to set, change, and delete any JSON property.
To access JSON object properties, you must use object notation names (such as "root.othernode.myproperty").

One advantage of JSONMaker is the ability to use different data stores to save your data. You can, for example, use a folder on your disk to store data. Every time you create an instance of JSONMaker to work with this folder, you can read and write this data, which is persistent.

JSONMaker can also work with memory. This storage method is much faster than using a folder, but this is not persistent.

We recommend that you use the folder when you need persistent data such as a cache or a configuration system (even a small database may be possible with this) and use memory for regular JSON uses (IPC, temporary storage in RAM)

## C#
`JSON jm = new JSON();`<br/>
`jm.setString("MyObject.Child.Child2.property", "example");`<br/>
`Console.WriteLine(jm.ToString());`<br/>
<br/>

## C++ 

![](https://i.imgur.com/f1a0cx1.png)

`JSON *jm = new JSON();`<br/>
`jm->setString("MyObject.Child.Child2.property", "example");`<br/>
`cout << jm.ToString());`<br/>




