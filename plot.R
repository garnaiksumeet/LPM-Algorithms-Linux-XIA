library(ggplot2)

data = read.table("./bloom_lookup_measurements", sep = "\t", col.names = c("FIB-SIZE", "TIME"), fill = F, strip.white = F)
df = as.data.frame(data)
fval = aggregate(df$TIME ~ df$FIB.SIZE, df, mean)
colnames(fval) <- c("FIB", "TIME")
fval$FIB <- factor(fval$FIB, labels = c("FIB"))

tiff("bloom.tif", units="in", width=11, height=8.5, res=300)
ggplot(fval, aes(x=FIB, y=TIME, color=FIB, fill=FIB)) + geom_boxplot(varwidth = T, size = 2.0) + xlab("FIB Size") + ylab("Average Time (in nanosec)") + ggtitle("Bloom Filters Lookup Performance for XID IDs")
dev.off()
