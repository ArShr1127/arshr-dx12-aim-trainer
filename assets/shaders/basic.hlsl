cbuffer SceneConstants : register(b0)
{
    row_major float4x4 gWorldViewProjection;
    row_major float4x4 gWorld;
    float4 gCameraPosition;
    float4 gModelTint;
    float4 gOptions;
    float4 gLightDirectionIntensity;
    float4 gLightColorAmbient;
};

struct VSInput
{
    float3 position : POSITION;
    float4 color : COLOR;
    float3 normal : NORMAL;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float3 normal : NORMAL;
    float3 worldPosition : TEXCOORD0;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    const float4 worldPosition = mul(float4(input.position, 1.0f), gWorld);

    output.position = mul(float4(input.position, 1.0f), gWorldViewProjection);
    output.color = input.color * gModelTint;
    output.normal = normalize(mul(float4(input.normal, 0.0f), gWorld).xyz);
    output.worldPosition = worldPosition.xyz;
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    if (gOptions.w < -0.5f)
    {
        return float4(input.color.rgb, input.color.a);
    }

    const float3 normal = normalize(input.normal);
    const float3 lightDirection = normalize(gLightDirectionIntensity.xyz);
    const float3 viewDirection = normalize(gCameraPosition.xyz - input.worldPosition);
    const float3 halfVector = normalize(lightDirection + viewDirection);

    const float diffuse = saturate(dot(normal, lightDirection)) * gLightDirectionIntensity.w;
    const float specular = pow(saturate(dot(normal, halfVector)), 32.0f) * 0.22f * gLightDirectionIntensity.w;
    const float selectedPulse = gOptions.x * (0.16f + 0.08f * sin(gOptions.z * 5.0f));
    const float pulse = 0.04f * sin(gOptions.z * 2.0f);

    const float3 lightColor = gLightColorAmbient.rgb;
    const float ambient = gLightColorAmbient.w;
    const float3 litColor = input.color.rgb * ((ambient + diffuse * 0.76f + pulse + selectedPulse) * lightColor) + specular.xxx;
    return float4(litColor, input.color.a);
}
