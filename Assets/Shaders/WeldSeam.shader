Shader "SpaceLite/WeldSeam"
{
    Properties
    {
        _MainTex ("Base (RGB)", 2D) = "white" {}
        _SeamMask ("Seam Mask (R)", 2D) = "white" {}
        _SeamColorIron ("Iron Seam Color", Color) = (0.8,0.6,0.5,1)
        _SeamColorAlu ("Aluminum Seam Color", Color) = (0.95,0.95,0.9,1)
        _DiscolorStrength ("Discolor Strength", Range(0,1)) = 0.6
        _MaterialType ("Material Type (0=Iron,1=Aluminum)", Range(0,1)) = 0
        _NoiseTex ("Noise", 2D) = "white" {}
        _GrindDetail ("Grind Detail Intensity", Range(0,2)) = 0.5
        _EdgeGlowIntensity ("Stress Glow", Range(0,2)) = 0.0
    }
    SubShader
    {
        Tags { "RenderType"="Opaque" }
        LOD 200

        CGPROGRAM
        #pragma surface surf Standard fullforwardshadows
        #pragma target 3.0

        sampler2D _MainTex;
        sampler2D _SeamMask;
        sampler2D _NoiseTex;
        fixed4 _SeamColorIron;
        fixed4 _SeamColorAlu;
        float _DiscolorStrength;
        float _MaterialType;
        float _GrindDetail;
        float _EdgeGlowIntensity;

        struct Input {
            float2 uv_MainTex;
            float2 uv_SeamMask;
            float2 uv_NoiseTex;
        };

        void surf (Input IN, inout SurfaceOutputStandard o)
        {
            fixed4 baseCol = tex2D(_MainTex, IN.uv_MainTex);
            float seam = tex2D(_SeamMask, IN.uv_SeamMask).r;

            // noise to create bead/texture
            float noise = tex2D(_NoiseTex, IN.uv_NoiseTex * 4.0).r;
            float bead = smoothstep(0.45, 0.65, noise) * seam;

            // choose seam color by material type
            fixed4 seamCol = lerp(_SeamColorIron, _SeamColorAlu, _MaterialType);

            // heat discoloration (iron: blue/purple/brown gradient approximated by tinting)
            fixed4 heatTint;
            if (_MaterialType < 0.5)
            {
                // iron: cool->hot: bluish to purple to brown-ish
                float t = noise;
                heatTint = lerp(fixed4(0.4,0.6,1.0,1.0), fixed4(0.6,0.4,0.8,1.0), t);
                heatTint = lerp(heatTint, fixed4(0.6,0.45,0.35,1.0), smoothstep(0.8,1.0,t));
            }
            else
            {
                // aluminum: light white-ish oxidation
                heatTint = fixed4(0.95,0.95,0.95,1.0) * (1.0 - _DiscolorStrength * 0.5);
            }

            // grind / surface detail
            float grind = tex2D(_NoiseTex, IN.uv_NoiseTex * 30.0).r * _GrindDetail;

            // combine colors
            fixed4 finalSeam = seamCol * bead + heatTint * (_DiscolorStrength * seam);
            float metallic = 0.9 * seam;
            float smooth = 0.3 + 0.7 * (1.0 - grind);

            // edge glow (stress)
            float edgeGlow = seam * _EdgeGlowIntensity;

            // output
            o.Albedo = lerp(baseCol.rgb, finalSeam.rgb, seam * 0.9) + edgeGlow;
            o.Metallic = metallic;
            o.Smoothness = smooth;
            o.Normal = o.Normal; // rely on normal map if assigned in material
        }
        ENDCG
    }
    FallBack "Diffuse"
}