namespace Loudness
{
class Loudness
{
public:
    static
        // Calculate calculates the average loudness level of a given dataset and puts it
        // into a linear form
        float
        Calculate(float* data, int dataSize, float mindB, float maxdB, float fftSizeInDB)
    {
        float average = 0.0f;

        /*
          in the following loop, we should try to put the level onto a linear scale, where
          0.5 feels approximately twice as oud at 1.0
      */
        for (int i = 0; i < dataSize; ++i)
        {
            auto gain = data[i];
            auto level = jmap(gain, 0.0f, 25.0f, 0.0f, 1.0f);
            average += level;
        }

        return average / dataSize;
    }
};
} // namespace Loudness
