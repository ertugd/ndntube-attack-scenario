#!/usr/bin/env Rscript
#################################################################
#Rates Script, Ertugrul Dogruluk- 2019
#Algorithmi Research Centree, Braga-Portugal, University of Minho
##################################################################
args = commandArgs(TRUE)
if (length(args) == 0) {
  cat ("ERROR: Scenario parameters should be specified\n")
  q(status = 1)
}

prefix = args[1]
topo   = args[2]
evil   = args[3]
runs   = args[4]
folder = args[5]
if (is.na (folder)) {
  folder = "newrun"
}
good   = args[6]
producer = args[7]

options(gsubfn.engine = "R")
suppressPackageStartupMessages (library(sqldf))
suppressPackageStartupMessages (library(ggplot2))
suppressPackageStartupMessages (library(reshape2))
suppressPackageStartupMessages (library(doBy))
suppressPackageStartupMessages (library(grid))
suppressPackageStartupMessages (library(gridExtra))
suppressPackageStartupMessages (library(stats))
suppressPackageStartupMessages (library(dplyr))
suppressPackageStartupMessages (library(base))
suppressPackageStartupMessages (library(scales))


source ("graphs/graph-style.R")

name = paste (sep="-", prefix, "topo", topo, "evil", evil, "good", good, "producer", producer)
dir.create (paste(sep="", "graphs/pdfs/", folder, "/"), showWarnings = FALSE, recursive = TRUE)
name2 = paste (sep="-","Cache-Hit-Misses", prefix, "topo", topo, "evil", evil, "producer", producer)
graphfile = paste(sep="","graphs/pdfs/", folder, "/", name2,".eps")

cat ("Writing to", graphfile, "\n")
pdf (graphfile)
#setEPS(graphfile, width=10, height=7.5, horizontal = FALSE, onefile = FALSE, paper = "special") 
#pdf (graphfile, width=5, height=4, paper='special')


for (run in strsplit(runs,",")[[1]]) {  
 # grid.newpage()
  filename <- paste (sep="", "results/", folder, "/", name, "-run-", run)
  #filename <- paste (sep="", "results/", folder, "/", name, "-run-1", run) #just analyze certain run
  
  cat ("Reading from", filename, "\n")

  input <- paste (sep="", filename, ".db")
  data.allFaces <- sqldf("select * from data", dbname = input, stringsAsFactors=TRUE)
  
  # data$Node <- factor(data$Node)
  ## data$FaceId <- factor(data$FaceId)
 # data.allFaces$Packets <- data.allFaces$Packets /100
  data.allFaces$Packets = as.numeric(data.allFaces$Packets) 
  
  # data.allFaces = summaryBy (. ~ Time + Node + Type, data=data, FUN=sum)

  xmin = min(data.allFaces$Time)
  xmax = max(data.allFaces$Time)

 
  
  graph <- function (data, variable='Packets') {
    #data <- subset(data, Type %in% c("InData", "OutInterests")) 
    data <- subset(data, Type %in% c("CacheMisses", "CacheHits")) 
    g <- ggplot (data,
                 aes(x=Time))
    g <- g + geom_point (aes_string(y=variable, color="Type"), size=0.4)
  #   g <- g + geom_point (aes(y=Packets.sum, color=Type), size=1)
    g <- g + scale_y_continuous (trans="log", labels = round)
    g <- g + scale_color_manual (values=c( "red", "darkgreen"))
    g <- g + facet_wrap (~ Node, nrow=3, ncol = 3)
    #g <- g + ggtitle ("Packets")
    #g <- g + theme(legend.justification=c(1,0), legend.position=c(1,0))+

    g <- g + theme_bw(base_size = 8)
    #g <- g +  scale_type(variable)
    #g <- g + theme(legend.position="bottom") # or "none" for no side print for Type values
  #  g <- g + scale_y_continuous(breaks=seq(0, 1500, 40)) #for small-tree and tree comment it. For ISP it is neccessary!. 
 # g <- g+  theme(legend.position = c(.95, .95),  legend.justification = c("right", "top"),  legend.box.background = element_rect())
      
    
   #g <- g + coord_cartesian(ylim=c(-10,1010))
  }
  
  ## data.good = subset ()
  #nodes.good = c(levels(data.allFaces$Node)[ grep ("^gw-", levels(data.allFaces$Node)) ])
  #nodes.good = sample (nodes.good, min(length(nodes.good), 1))
  ## nodes.good = c("good-leaf-12923")
  
  nodes.evil = levels(data.allFaces$Node)[ grep ("^bb-", levels(data.allFaces$Node)) ] #for testbed its bb, for isp its gw!!!!!!!!
  nodes.evil = sample (nodes.evil, min(length(nodes.evil), 9))

 # nodes.producer = levels(data.allFaces$Node)[ grep ("^gw-", levels(data.allFaces$Node)) ]
  #nodes.producer = sample (nodes.producer, min(length(nodes.producer), 1))
  
  #g1 <- graph (subset (data.allFaces, Node %in% nodes.good),     "Packets")
  g2 <- graph (subset (data.allFaces, Node %in% nodes.evil),     "Packets")
  #g3 <- graph (subset (data.allFaces, Node %in% nodes.producer), "Packets")
  print(g2)
#  pushViewport(vpList(
    #viewport(x = 0.5, y = .66, width = 1, height = .33,
     # just = c("center", "bottom"), name = "p1"),
    #viewport(x = 0.5, y = .33, width = 1, height = .33,
    #  just = c("center", "bottom"), name = "p2")
    #viewport(x = 0.5, y = .00, width = 1, height = .33,
    #  just = c("center", "bottom"), name = "p3")
   # ))
  
  ## Add the plots from ggplot2
  #upViewport()
  #downViewport("p1")
  #print(g1, newpage = FALSE)
  
#  upViewport()
 # downViewport("p2")
  #print(g2, newpage = FALSE)
  
  #upViewport()
  #downViewport("p3")
  #print(g3, newpage = FALSE)
  
 # grid.newpage()
  
 # g1 <- graph (subset (data.allFaces, Node %in% nodes.good),     "Packets")
  #g2 <- graph (subset (data.allFaces, Node %in% nodes.evil),     "Packets")
  #g3 <- graph (subset (data.allFaces, Node %in% nodes.producer), "Packets")
    
#  pushViewport(vpList(
  #  viewport(x = 0.5, y = .66, width = 1, height = .33,
  #    just = c("center", "bottom"), name = "p1"),
 #   viewport(x = 0.5, y = .33, width = 1, height = .33,
  #    just = c("center", "bottom"), name = "p2")
   # viewport(x = 0.5, y = .00, width = 1, height = .33,
  #    just = c("center", "bottom"), name = "p3")
   # ))
  ## Add the plots from ggplot2
 # upViewport()
  #downViewport("p1")
  #print(g1, newpage = FALSE)
  
#  upViewport()
  #downViewport("p2")
 # print(g2, newpage = FALSE)
  
  #upViewport()
  #downViewport("p3")
  #print(g3, newpage = FALSE)  
}

x = dev.off ()
cat ("Done\n")
