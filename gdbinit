display/z $x5
display/z $x6
display/z $x7
display/z $x28
display/z $x29
display/z $x30
display/z $x31

set disassemble-next-line on
b main
target remote: 1234
c