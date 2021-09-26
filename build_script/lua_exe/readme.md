如果你希望编译lua解释器，并且这个解释器能够被emmylua 调试，那么只需要按照标准的方式先编译出lua dll 再链接生成lua可执行文件就行了。
这里提供简单的powershell脚本

Q:如果我希望编译linux 和macosx下的lua解释器怎么做？
A:自己改个后缀为.sh 再改下换行为LF 就能用了