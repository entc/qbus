import qbus

def main_init(qbus):
  print("init")
  
def main_done(obj):
  print("done")

def main():
  qbus.instance("EXAMPLE", main_init, main_done)

if __name__ == "__main__":
  main()
