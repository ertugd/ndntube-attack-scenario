#!/usr/bin/env Rscript
#################################################################
#min.max.cache hit ratios, Ertugrul Dogruluk- 2019
#Algorithmi Research Centree, Braga-Portugal, University of Minho
##################################################################
args = commandArgs(TRUE)
script_mode= TRUE;
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
  script_mode = FALSE;
  cat("Not in script mode, outputing graph to default device...\n")
  # close devices...
  while (dev.cur() > 1) dev.off()
  # create a default device
  dev.new()
} 

prefixes = args[1]
topo     = args[2]
evils    = args[3]
runs     = args[4]
folder   = args[5]
good     = args[6]
producer = args[7]


# use tidyverse only : there is a proble using plyr instead of dplyr
# you may have to install using: install.packages("tidyverse")
suppressPackageStartupMessages (library(tidyverse))
#suppressPackageStartupMessages (library(gridExtra))
# don't include anything else.. is possible

source ("graphs/graph-style.R")

name = paste (sep="-", prefixes, "topo", topo, "good", good, "evil", evils, "producer", producer)
filename = paste(sep="", "results/",folder,"/process/", name, "-all-data.dat")

if (file_test("-f", filename)) {
  cat ("Loading data from", filename, "\n")
  load (filename)
  
} else {
  data.all = data.frame ()
  for (evil in strsplit(evils,",")[[1]]) {
    for (prefix in strsplit(prefixes,",")[[1]]) {
      name = paste (sep="-", prefix, "topo", topo, "evil", evil, "good", good, "producer", producer)
      filename = paste(sep="", "results/",folder,"/process/", name, ".txt")
      cat ("Reading from", filename, "\n")
      ## data = read.table (filename, header=TRUE)
      load (filename)
      
      data.all <- rbind (data.all, data)
    }
  }
  
  name = paste (sep="-", prefixes, "topo", topo, "good", good, "evil", evils, "producer", producer)
  filename = paste(sep="", "results/",folder,"/process/", name, "-all-data.dat")
  
  cat ("Saving data to", filename, "\n")
  save (data.all, file=filename)
}

name2 = paste (sep="-", "CRT-avarage", "topo", topo, "evil", evils, "producer", producer)

data.all$Scenario = ordered (data.all$Scenario,
                             c("Lru1","Lru2","Fifo","Lfu","Probability1","Probability2","Random","Freshness","dad1","dad2"))

#levels(data.all$Scenario) <- sub("^nocache$", "no-cache", levels(data.all$Scenario))
levels(data.all$Scenario) <- sub("^dad1$", "nfd:DaD1", levels(data.all$Scenario)) #best-route
levels(data.all$Scenario) <- sub("^dad2$", "nfd:DaD", levels(data.all$Scenario)) #multicast
levels(data.all$Scenario) <- sub("^Freshness$", "Randomized cache with Freshness (100ms)", levels(data.all$Scenario))
levels(data.all$Scenario) <- sub("^Random$", "Randomized cache", levels(data.all$Scenario))
levels(data.all$Scenario) <- sub("^Probability1$", "nfd:Probabilistic1", levels(data.all$Scenario)) #best-route
levels(data.all$Scenario) <- sub("^Probability2$", "nfd:Probabilistic", levels(data.all$Scenario)) #multi-cast
levels(data.all$Scenario) <- sub("^Lfu$", "nfd:LFU", levels(data.all$Scenario))
levels(data.all$Scenario) <- sub("^Fifo$", "nfd:FIFO", levels(data.all$Scenario))
levels(data.all$Scenario) <- sub("^Lru2$", "nfd:LRU", levels(data.all$Scenario)) #multicast
levels(data.all$Scenario) <- sub("^Lru1$", "nfd:LRU1", levels(data.all$Scenario)) #best-route


#minTime = 300
minTime=0
  attackTime = 30

#gdata = subset(data.all, minTime-0 <= Time & Time < minTime+attackTime+0)
#cat("Gdata = ", nrow(data.all), "\n")
#gsample = ggdata[sample(nrow(ggdata), length(ggdata$Time)/50),]

# lets do some agregation on data using time intervals...
# first create a tibble 
g1 <- as_tibble(data.all)
g1evil <- filter(g1, NodeType == "Adversaries")
g1good <- filter(g1, NodeType == "Legitimates")
g1sum = g1 %>% 
        mutate(DeltaTime = round(Time, digits =1)) %>% 
        group_by(Scenario, Run, Node, NodeType, DeltaTime) %>% 
        summarise(
          MeanDelay=mean(DelayS, na.rm = TRUE), 
          MeanHopCount=mean(HopCount, na.rm=TRUE)
        )
cat(names(g1))
cat("Data summarized(", round(nrow(g1sum)*10/nrow(g1), digits=1), "%): ", 
    nrow(g1), " rows were summarized to ", nrow(g1sum)," \n")
cat(names(g1sum))
g1total <- g1 %>%  
  filter((Time >= 21) & (Time<= 40)) %>% 
  group_by(Scenario, NodeType) %>% 
        summarise(
           MeanDelay=mean(DelayS, na.rm = TRUE), 
           MeanHopCount=mean(HopCount, na.rm=TRUE)
        )
print(g1total)

#-------------------------------------------------------------------------
#build the graphs...
# decide if we want to send to file or to screen
if (script_mode) {
  cat (sep="", "Writing to ", paste(sep="","pdfs/", folder, "/",name2,"_CRT.eps"))
#  pdf (paste(sep="","graphs/pdfs/", folder, "/",name2,"_CRT.eps"), width=4.89, height=10) #ieee-access 
  pdf (paste(sep="","graphs/pdfs/", folder, "/",name2,"_CRT.eps"), width=7, height=12)
} 

# create the graph for MeanDelay... (CRT)
g <- ggplot(g1sum, aes(x=DeltaTime, y=MeanDelay, color=NodeType, shape=NodeType)) + 
#  geom_point(data=filter(g1sum, NodeType==c("Adversaries")), aes(alpha=NodeType), show.legend=F,size=0.1) + 
  geom_point(data=filter(g1sum, NodeType==c("Adversaries", "Legitimates")), show.legend=F,size=0.1) + 
  
  geom_smooth(method="auto", se=F) + 
  xlim(c(21,36)) +
  scale_y_log10() +
  theme_bw(base_size = 16)+
  theme(legend.position = "bottom")+
  guides(color = guide_legend(order=1))+
  facet_wrap(~Scenario, nrow = 3, ncol = 1) +
  labs(
    #title="Content Retrieval Time (CRT) Analysis", 
    #subtitle="Mean CRT vs Time ", 
    y="CRT values (ms)", 
    x="Attack time (seconds)")+
  # caption = "VoNDN App" )  
  #  scale_color_discrete(name = " ", labels = c("Adversary","Legitimate"))
 scale_color_discrete(name = "VoNDN", labels = c("Adversary","Legitimate"))
# create the graph
print(g)

#-------------------------------------------------------------------------
#build the graphs...
# decide if we want to send to file or to screen
if (script_mode) {
  cat (sep="", "Writing to ", paste(sep="","pdfs/", folder, "/",name2,"_HC.eps"))
 # pdf (paste(sep="","graphs/pdfs/", folder, "/",name2,"_HC.eps"), width=12, height=8) #thesis
  pdf (paste(sep="","graphs/pdfs/", folder, "/",name2,"_HC.eps"), width=7, height=10)
  
} 

# create the graph for HopCount (HC)
g <- ggplot(g1sum, aes(x=DeltaTime, y=MeanHopCount, color=NodeType, shape=NodeType)) + 
  geom_point(data=filter(g1sum, NodeType==c("Adversaries", "Legitimates")), show.legend=F,size=0.1) + 
  geom_smooth(method="auto", se=F) + 
  xlim(c(21,36)) +
    facet_wrap(~Scenario, nrow = 1, ncol = 3) +
    theme_bw(base_size = 16)+
    scale_y_log10() +
    theme(legend.position = "bottom")+
  #scale_shape_discrete(name = " ", labels = c("Adversary","Legitimate"))+
 # scale_color_discrete(name = " ", labels = c("Adversary","Legitimate"))+
     labs(
 #      title="Hop-Count Analysis", 
#       subtitle="Average Hop Count vs Time ", 
       y="Hop Count (number)", 
       x="Attack time (seconds)"
     #  caption = "VoNDN"
     )+
 scale_color_discrete(name = "VoNDN", labels = c("Adversary","Legitimate"))


# create the graph
print(g)

if (script_mode) {
  x = dev.off ()
}

