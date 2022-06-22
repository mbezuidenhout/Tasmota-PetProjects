const char HTTP_HEADER1[] PROGMEM =
  "<!DOCTYPE html><html lang=\"%s\" class=\"\">"
  "<head>"
  "<meta charset='utf-8'>"
  "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1,user-scalable=no\"/>"
  "<link rel=\"icon\" href=\"data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAYAAABzenr0AAAACXBIWXMAAA7EAAAOxAGVKw4bAAACeklEQVRYhe2WS08TURiGn7n03k7lYkULUSKNIhAWBoO4cGVCSFyoiYpr/4J/yhVihET3aJCVCYaCJApybTBtp9PLXM5x19AIdAixLOi7Pd933ifvfOecUbrfPJCco9TzNG8DtAEuFoCoODjbJlI0nnq9FeZuzsKczaKlYgjLJvl8GEVVgBYlUMseEJ3ow3h8CzWk4/2p1NdakkAgnaCyuIV0BF6+ipYM1df+ewLCspG2R2S8F1FzMZ7dQQlorQGQUmLOryFtj+D1S0TH0mhGqKHGN4CzW6L6bRdh2b4BqkvbqGGd0HDq2BpfAM5mAevjOsJyKLxdRrrCJ/AesUc3URTlbAD2RoHI2DWi9/vQOiMNU3yUhO1RmlslPpVBDZ08575OQWigE3P+B86OiTBraF2RE+utT+uEhlIEriaa7t00AXevhL1RwHg6SLC/g+SLYcy5NZyt4pH11eV9hOUQuZduat4UwM1ZFGezBHoNNCNEsL8DJaARHe+lNL9GbSXXUO/lq5QXNklMZU787r4A3IMyxZkVElOZf6LUL8dIvhyh8nUb96CM/TNP+ctvpCcwngyixoO+zI8EkFIiPYH5PkticoBA2ji6MR4k+WoEUaxRXthECeuYMyuohn9zODSEXr5K8d0KsuYSHu0hOT3SdIIVTcXdKREevUJ4KIXzK4/I11BT/m/4egKVpS2iE310vL5L7XvupJ4GBW93UVncwvywiqw4aN1R371wKAE1FsTdt9CSYZASRfd3SeqdUZLTI4h8FS0Vqz+zflV3iYylwRWUP28Sn8ygaP6fCTWso/fET20OhxJQdJXYwxun3uCsujj/hG2ANsBx+gtvhui6jAiZ+AAAAABJRU5ErkJggg==\">"
  "<title>%s - %s</title>"

  "<script>"
  "var x=null,lt,to,tp,pc='';"            // x=null allow for abortion

// Following bytes saving ES6 syntax fails on old browsers like IE 11 - https://kangax.github.io/compat-table/es6/
  "eb=s=>document.getElementById(s);"     // Alias to save code space
  "qs=s=>document.querySelector(s);"      // Alias to save code space
  "sp=i=>eb(i).type=(eb(i).type==='text'?'password':'text');"  // Toggle password visibility
  "wl=f=>window.addEventListener('load',f);" // Execute multiple window.onload
  ;
