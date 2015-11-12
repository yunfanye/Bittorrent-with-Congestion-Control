output_1 = "1.haschunks"
output_2 = "2.haschunks"
output_3 = "master.haschunks"
out_file = open(output_1, "w")
chunks = 2000
for i in range(chunks):
	out_file.write(str(i)+" "+"3b9f916bbf59021ab781c9f2456df90a0102079f"+"\n")

out_file = open(output_2, "w")
for i in range(chunks):
	out_file.write(str(i)+" "+"78585121ee33fbb6666bc4bf1a8498c04b2758e8"+"\n")

out_file = open(output_3, "w")
out_file.write("File: NOEXIST.tar\n")
out_file.write("Chunks:\n")
for i in range(chunks):
	out_file.write(str(i)+" "+"3b9f916bbf59021ab781c9f2456df90a0102079f"+"\n")
for i in range(chunks):
	out_file.write(str(chunks+i)+" "+"78585121ee33fbb6666bc4bf1a8498c04b2758e8"+"\n")