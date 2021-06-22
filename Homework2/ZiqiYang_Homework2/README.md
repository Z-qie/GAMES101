ref[MSAA中黑线问题的出现原因以及解决方案]: (http://games-cn.org/forums/topic/%E3%80%90%E6%80%BB%E7%BB%93%E3%80%91msaa%E4%B8%AD%E9%BB%91%E7%BA%BF%E9%97%AE%E9%A2%98%E7%9A%84%E5%87%BA%E7%8E%B0%E5%8E%9F%E5%9B%A0%E4%BB%A5%E5%8F%8A%E8%A7%A3%E5%86%B3%E6%96%B9%E6%A1%88/)

Implementation：

![withoutMSAA](withoutMSAA.png)

- rasterization with bounding box
- check if any point is inside the triangle（change the first two params. as float）
- z-buffer
- MSAA 
  - maintain extra MSAA depth_buffer & color_buffer (to mix up final colors to eliminate abnormal line at the intersection btw two tris.)
  - more like an SSAA
  - press 'a' to switch the mode btw MSAA and normal case

![withMSAA.png](withMSAA.png)
