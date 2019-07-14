// automatically generated by the FlatBuffers compiler, do not modify

package MyGame;

import java.nio.*;
import java.lang.*;
import java.util.*;
import com.google.flatbuffers.*;

@SuppressWarnings("unused")
public final class PolarPoint extends Struct {
  public void __init(int _i, ByteBuffer _bb) { __reset(_i, _bb); }
  public PolarPoint __assign(int _i, ByteBuffer _bb) { __init(_i, _bb); return this; }

  public float mag() { return bb.getFloat(bb_pos + 0); }
  public void mutateMag(float mag) { bb.putFloat(bb_pos + 0, mag); }
  public float arg() { return bb.getFloat(bb_pos + 4); }
  public void mutateArg(float arg) { bb.putFloat(bb_pos + 4, arg); }

  public static int createPolarPoint(FlatBufferBuilder builder, float mag, float arg) {
    builder.prep(4, 8);
    builder.putFloat(arg);
    builder.putFloat(mag);
    return builder.offset();
  }
}

