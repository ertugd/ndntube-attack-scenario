#!/usr/bin/env Rscript
#################################################################
#pre-process Script, Ertugrul Dogruluk- 2019
#Algorithmi Research Centree, Braga-Portugal, University of Minho
##################################################################
args = commandArgs(TRUE)
if (length(args) == 0) {
  cat ("WARNING: Scenario parameters should be specified\n")
  cat ("Proceeding any way with default values: hardcoded ones!\n")
  args[1] = "Lru1,Probability1,dad1"
  args[2] = "testbed"
  args[3] = "168"
  args[4] = "1"
  args[5] = "attack_vondn"
  args[6] = "0"
  args[7] = "bb"
  ## q(status = 1)
  cat(args, "\n")
} 

options(gsubfn.engine = "R")
suppressPackageStartupMessages (library(sqldf))
suppressPackageStartupMessages (library(ggplot2))
suppressPackageStartupMessages (library(reshape2))
suppressPackageStartupMessages (library(doBy))

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

name = paste (sep="-", prefix, "topo", topo, "evil", evil, "good", good, "producer", producer)

data.all = data.frame ()

for (run in strsplit(runs,",")[[1]]) {  
  filename <- paste (sep="", "results/", folder, "/", name, "-run-", run)
  cat ("Reading from", filename, "\n")
  
  input <- paste (sep="", filename, ".db")
  data.run <- sqldf("select * from data", dbname = input, stringsAsFactors=TRUE)
  
  nodes.good     = levels(data.run$Node)[ grep ("^good-", levels(data.run$Node)) ]
  nodes.evil     = levels(data.run$Node)[ grep ("^evil-", levels(data.run$Node)) ]
  #nodes.producer = levels(data.run$Node)[ grep ("^gw-", levels(data.run$Node)) ]
  
  run.ratios <- function (data) {
    data.evilmisses = subset (data, Node %in% nodes.good & Type == "FullDelay")
    data.evilmisses$Type <- NULL
    names(data.evilmisses)[names(data.evilmisses) == "Type"] = "FullDelay"
    
    
    data.evilhits      = subset (data, Node %in% nodes.good & Type == "FullDelay")
    data.evilhits$Type <- NULL
    names(data.evilhits)[names(data.evilhits) == "Type"] = "FullDelay"
    
    
    data.out = merge (data.evilmisses, data.evilhits)
    data.out$Run  = run
    data.out$Ratio = data.out$FullDelay 
    #   data.out$Ratio = 10 
    data.out
  }
  
  run.fulldelay <- function (data) {
    data.goodguys= subset (data, Node %in% nodes.good & Type == "FullDelay")
    #   cat("Goodguys lines...", nrow(data.goodguys), "\n")
    data.goodguys$Type <- NULL
    data.goodguys$NodeType = "Legitimates"
    names(data.goodguys)[names(data.goodguys) == "Type"] = "FullDelay"
    
    data.badguys= subset (data, Node %in% nodes.evil & Type == "FullDelay")
    #    cat("Badguys lines...", nrow(data.badguys), "\n")
    data.badguys$Type <- NULL
    data.badguys$NodeType = "Adversaries"
    names(data.badguys)[names(data.badguys) == "Type"] = "FullDelay"
    
    data.out = rbind (data.goodguys, data.badguys)
    data.out$Run  = run
    data.out
  }
  
  run.hopcount <- function (data) {
    data.out = subset (data, Node %in% c("nodes.good","nodes.evil") )
    data.out$Run  = run
    data.out$Ratio = 0 
    data.out
  }
  #   use this line if you want to compute cache hit rations  
  #  cat ("Merging...", run, "\n")
  
  data.all = rbind (data.all, run.fulldelay (data.run))
  #  cat ("Merged...", run, "\n")
  
  #   or this one for just collecting hopcount
  #data.all = rbind (data.all, run.hopcount(data.run))
  
}

## data.all$Type = factor(data.all$Type)
data.all$Run  = factor(data.all$Run)
data.all$Scenario = factor(prefix)
data.all$Evil     = factor(evil)
data.all$Good     = factor(good)


outputfile = paste(sep="", "results/",folder,"/process/", name, ".txt")
unlink(outputfile)
#unlink(paste(sep="", "results/",folder,"/process/"), recursive = TRUE)
dir.create (paste(sep="", "results/",folder,"/process/"), showWarnings = FALSE)
cat (">> Writing", outputfile, "\n")
## write.table(data.all, file = outputfile, row.names=FALSE, col.names=TRUE)
data = data.all
save (data, file=outputfile)

