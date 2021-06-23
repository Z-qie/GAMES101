Implementation：

[5 marks] all file expected submitted with normal running behavior.

[10 marks] correctly implementation of interpolation on color, normal vector, texture coordinate and shading position (world space) and passing to fragment_shader_payload.

<img src="images/normal_shader.png" alt="normal_shader"  />

[20 marks] correctly implemented Blinn-Phong reflectance model.

(2 times ambient light)

![blinn_phong](images/blinn-phong.png)

[5 marks] correctly implemented Texture mapping with Blinn-Phong reflectance model

![texture](images/texture.png)

[5 marks] correctly implemented Bump mapping 

[using texture pic as normal texture]

![bt](images/bumping_texture.png)

[using hmap as normal texture]

![bt](images/bumping_hmap.png)

[5 marks] correctly implemented Displacement mapping

![d](images/displacement.png)

[Bonus 5 marks] correctly implemented getColorBilinear defined in Texture class and used in texture_fragment_shader with test by low fidelity pic (300*300)

[300*300p without bilinear]

![low_res_bilinear](images/low_res.png)

[300*300p with bilinear]

![low_res_bilinear](images/low_res_bilinear.png)

