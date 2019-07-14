// <auto-generated>
//  automatically generated by the FlatBuffers compiler, do not modify
// </auto-generated>

namespace MyGame
{

using global::System;
using global::FlatBuffers;

public struct Transformation : IFlatbufferObject
{
  private Struct __p;
  public ByteBuffer ByteBuffer { get { return __p.bb; } }
  public void __init(int _i, ByteBuffer _bb) { __p = new Struct(_i, _bb); }
  public Transformation __assign(int _i, ByteBuffer _bb) { __init(_i, _bb); return this; }

  public float Scale { get { return __p.bb.GetFloat(__p.bb_pos + 0); } }
  public void MutateScale(float scale) { __p.bb.PutFloat(__p.bb_pos + 0, scale); }
  public float Angle { get { return __p.bb.GetFloat(__p.bb_pos + 4); } }
  public void MutateAngle(float angle) { __p.bb.PutFloat(__p.bb_pos + 4, angle); }

  public static Offset<MyGame.Transformation> CreateTransformation(FlatBufferBuilder builder, float Scale, float Angle) {
    builder.Prep(4, 8);
    builder.PutFloat(Angle);
    builder.PutFloat(Scale);
    return new Offset<MyGame.Transformation>(builder.Offset);
  }
};


}
