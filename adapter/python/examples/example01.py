import qbus

def main_get(qin):
  print (qin)

def main_put(qin):
  print (qin)

def main_init(qbus):
  
  qbus.register ("bts", main_get)
  qbus.register ("PUT", main_put)
    
def main_done(qbus, obj):
  
  print("done")
  
  print(obj)

def main():

  qbus.instance("VELE", main_init, main_done)

if __name__ == "__main__":
  main()
