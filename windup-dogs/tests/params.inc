static wd::Params toParams (const float* v)
{
    wd::Params q;
    q.inst = (wd::Inst) (int) v[0]; q.tone = v[1]; q.meca = v[2]; q.wobble = v[3]; q.echo = v[4]; q.space = v[5]; q.levelDb = v[6];
    q.attackMs = v[7]; q.decayMs = v[8]; q.sustain = v[9]; q.releaseMs = v[10]; q.bright = v[11]; q.tune = v[12];
    q.tickOn = v[13] > 0.5f; q.tickDiv = (int) v[14]; q.tickLevel = v[15]; q.swing = v[16]; q.springOn = v[17] > 0.5f; q.springTime = v[18];
    return q;
}
