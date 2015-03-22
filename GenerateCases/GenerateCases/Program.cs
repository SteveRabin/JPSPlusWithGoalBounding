using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace GenerateCases
{
    public enum Dir
    {
        Down        = 0,
        DownRight   = 1,
        Right       = 2,
        UpRight     = 3,
        Up          = 4,
        UpLeft      = 5,
        Left        = 6,
        DownLeft    = 7,
        All         = 8
    };

    class Program
    {
        static void Main(string[] args)
        {
            using (System.IO.StreamWriter file = new System.IO.StreamWriter(@"Cases.h"))

            for (int bits = 0; bits < 256; ++bits)
            {
                file.WriteLine("CASE(" + TranslateDown(Down(bits)) + ")");
                file.WriteLine("CASE(" + TranslateDownRight(DownRight(bits)) + ")");
                file.WriteLine("CASE(" + TranslateRight(Right(bits)) + ")");
                file.WriteLine("CASE(" + TranslateUpRight(UpRight(bits)) + ")");
                file.WriteLine("CASE(" + TranslateUp(Up(bits)) + ")");
                file.WriteLine("CASE(" + TranslateUpLeft(UpLeft(bits)) + ")");
                file.WriteLine("CASE(" + TranslateLeft(Left(bits)) + ")");
                file.WriteLine("CASE(" + TranslateDownLeft(DownLeft(bits)) + ")");
            }

        }

        static bool Blocked(int bits, Dir dir)
        {
            return (((bits) & (1 << (int)dir)) > 0);
        }

        static bool Empty(int bits, Dir dir)
        {
            return !Blocked(bits, dir);
        }

        static string Down(int bits)
        {
            Dir primaryDir = Dir.Down;
            bool straight = Empty(bits, Dir.Down);
            bool leftForcedNeighbor = Blocked(bits, Dir.UpRight) && Empty(bits, Dir.Right);
            bool rightForcedNeighbor = Blocked(bits, Dir.UpLeft) && Empty(bits, Dir.Left);
            bool leftForcedDiagonal = leftForcedNeighbor && straight && Empty(bits, Dir.DownRight);
            bool rightForcedDiagonal = rightForcedNeighbor && straight && Empty(bits, Dir.DownLeft);

            return GetCardinalVariation(
                bits,
                primaryDir,
                straight,
                leftForcedNeighbor,
                rightForcedNeighbor,
                leftForcedDiagonal,
                rightForcedDiagonal);
        }

        static string Right(int bits)
        {
            Dir primaryDir = Dir.Right;
            bool straight = Empty(bits, Dir.Right);
            bool leftForcedNeighbor = Blocked(bits, Dir.UpLeft) && Empty(bits, Dir.Up);
            bool rightForcedNeighbor = Blocked(bits, Dir.DownLeft) && Empty(bits, Dir.Down);
            bool leftForcedDiagonal = leftForcedNeighbor && straight && Empty(bits, Dir.UpRight);
            bool rightForcedDiagonal = rightForcedNeighbor && straight && Empty(bits, Dir.DownRight);

            return GetCardinalVariation(
                bits,
                primaryDir,
                straight,
                leftForcedNeighbor,
                rightForcedNeighbor,
                leftForcedDiagonal,
                rightForcedDiagonal);
        }

        static string Up(int bits)
        {
            Dir primaryDir = Dir.Up;
            bool straight = Empty(bits, Dir.Up);
            bool leftForcedNeighbor = Blocked(bits, Dir.DownLeft) && Empty(bits, Dir.Left);
            bool rightForcedNeighbor = Blocked(bits, Dir.DownRight) && Empty(bits, Dir.Right);
            bool leftForcedDiagonal = leftForcedNeighbor && straight && Empty(bits, Dir.UpLeft);
            bool rightForcedDiagonal = rightForcedNeighbor && straight && Empty(bits, Dir.UpRight);

            return GetCardinalVariation(
                bits,
                primaryDir,
                straight,
                leftForcedNeighbor,
                rightForcedNeighbor,
                leftForcedDiagonal,
                rightForcedDiagonal);
        }

        static string Left(int bits)
        {
            Dir primaryDir = Dir.Left;
            bool straight = Empty(bits, Dir.Left);
            bool leftForcedNeighbor = Blocked(bits, Dir.DownRight) && Empty(bits, Dir.Down);
            bool rightForcedNeighbor = Blocked(bits, Dir.UpRight) && Empty(bits, Dir.Up);
            bool leftForcedDiagonal = leftForcedNeighbor && straight && Empty(bits, Dir.DownLeft);
            bool rightForcedDiagonal = rightForcedNeighbor && straight && Empty(bits, Dir.UpLeft);

            return GetCardinalVariation(
                bits,
                primaryDir,
                straight,
                leftForcedNeighbor,
                rightForcedNeighbor,
                leftForcedDiagonal,
                rightForcedDiagonal);
        }

        static string GetCardinalVariation(
            int bits,
            Dir dir,
            bool straight,
            bool leftForcedNeighbor,
            bool rightForcedNeighbor,
            bool leftForcedDiagonal,
            bool rightForcedDiagonal)
        {
            string pre = "";

            if (straight && !leftForcedNeighbor && !rightForcedNeighbor)
            {
                return pre + "XX2XX";
            }
            else if (!straight && leftForcedNeighbor && !rightForcedNeighbor)
            {
                return pre + "0XXXX";
            }
            else if (!straight && !leftForcedNeighbor && rightForcedNeighbor)
            {
                return pre + "XXXX4";
            }
            else if (leftForcedDiagonal && !rightForcedNeighbor)
            {
                return pre + "012XX";
            }
            else if (rightForcedDiagonal && !leftForcedNeighbor)
            {
                return pre + "XX234";
            }
            else if (leftForcedDiagonal && rightForcedDiagonal)
            {
                return pre + "01234";
            }
            else if (straight && leftForcedNeighbor && !rightForcedNeighbor && !leftForcedDiagonal)
            {
                return pre + "0X2XX";
            }
            else if (straight && !leftForcedNeighbor && rightForcedNeighbor && !rightForcedDiagonal)
            {
                return pre + "XX2X4";
            }
            else if (!straight && leftForcedNeighbor && rightForcedNeighbor)
            {
                return pre + "0XXX4";
            }
            else if (straight && leftForcedNeighbor && rightForcedNeighbor && !leftForcedDiagonal && !rightForcedDiagonal)
            {
                return pre + "0X2X4";
            }
            else if (straight && leftForcedNeighbor && rightForcedNeighbor && leftForcedDiagonal && !rightForcedDiagonal)
            {
                return pre + "012X4";
            }
            else if (straight && leftForcedNeighbor && rightForcedNeighbor && !leftForcedDiagonal && rightForcedDiagonal)
            {
                return pre + "0X234";
            }
            else
            {
                return pre + "XXXXX";
            }
        }

        static string DownRight(int bits)
        {
            Dir primaryDir = Dir.DownRight;
            bool leftish = Empty(bits, Dir.Right);
            bool rightish = Empty(bits, Dir.Down);
            bool straight = leftish && rightish && Empty(bits, Dir.DownRight);

            return GetDiagonalVariation(
                primaryDir,
                straight,
                leftish,
                rightish);
        }

        static string UpRight(int bits)
        {
            Dir primaryDir = Dir.UpRight;
            bool leftish = Empty(bits, Dir.Up);
            bool rightish = Empty(bits, Dir.Right);
            bool straight = leftish && rightish && Empty(bits, Dir.UpRight);

            return GetDiagonalVariation(
                primaryDir,
                straight,
                leftish,
                rightish);
        }

        static string UpLeft(int bits)
        {
            Dir primaryDir = Dir.UpLeft;
            bool leftish = Empty(bits, Dir.Left);
            bool rightish = Empty(bits, Dir.Up);
            bool straight = leftish && rightish && Empty(bits, Dir.UpLeft);

            return GetDiagonalVariation(
                primaryDir,
                straight,
                leftish,
                rightish);
        }

        static string DownLeft(int bits)
        {
            Dir primaryDir = Dir.DownLeft;
            bool leftish = Empty(bits, Dir.Down);
            bool rightish = Empty(bits, Dir.Left);
            bool straight = leftish && rightish && Empty(bits, Dir.DownLeft);

            return GetDiagonalVariation(
                primaryDir,
                straight,
                leftish,
                rightish);
        }

        static string GetDiagonalVariation(
            Dir dir,
            bool straight,
            bool leftish,
            bool rightish)
        {
            string pre = "";

            if (leftish && !rightish)
            {
                return pre + "0XX";
            }
            else if (!leftish && rightish)
            {
                return pre + "XX2";
            }
            else if (straight)
            {
                return pre + "012";
            }
            else if (leftish && rightish && !straight)
            {
                return pre + "0X2";
            }
            else
            {
                return pre + "XXX";
            }
        }

        static string TranslateDown(string input)
        {
            if (input == "XX2XX") return "D";
            if (input == "0XXXX") return "R";
            if (input == "XXXX4") return "L";
            if (input == "0X2XX") return "D_R";
            if (input == "XX2X4") return "L_D";
            if (input == "0XXX4") return "R_L";
            if (input == "012XX") return "D_DR_R";
            if (input == "XX234") return "L_DL_D";
            if (input == "0X2X4") return "L_D_R";
            if (input == "012X4") return "R_DR_D_L";
            if (input == "0X234") return "R_D_DL_L";
            if (input == "01234") return "R_DR_D_DL_L";
            if (input == "XXXXX") return "Null";
            return "Null";
        }

        static string TranslateDownRight(string input)
        {
            if (input == "0XX") return "R";
            if (input == "XX2") return "D";
            if (input == "0X2") return "D_R";
            if (input == "012") return "D_DR_R";
            if (input == "XXX") return "Null";
            return "Null";
        }

        static string TranslateRight(string input)
        {
            if (input == "XX2XX") return "R";
            if (input == "0XXXX") return "U";
            if (input == "XXXX4") return "D";
            if (input == "0X2XX") return "R_U";
            if (input == "XX2X4") return "D_R";
            if (input == "0XXX4") return "D_U";
            if (input == "012XX") return "R_UR_U";
            if (input == "XX234") return "D_DR_R";
            if (input == "0X2X4") return "D_R_U";
            if (input == "012X4") return "U_UR_R_D";
            if (input == "0X234") return "U_R_DR_D";
            if (input == "01234") return "U_UR_R_DR_D";
            if (input == "XXXXX") return "Null";
            return "Null";
        }

        static string TranslateUpRight(string input)
        {
            if (input == "0XX") return "U";
            if (input == "XX2") return "R";
            if (input == "0X2") return "R_U";
            if (input == "012") return "R_UR_U";
            if (input == "XXX") return "Null";
            return "Null";
        }

        static string TranslateUp(string input)
        {
            if (input == "XX2XX") return "U";
            if (input == "0XXXX") return "L";
            if (input == "XXXX4") return "R";
            if (input == "0X2XX") return "U_L";
            if (input == "XX2X4") return "R_U";
            if (input == "0XXX4") return "R_L";
            if (input == "012XX") return "U_UL_L";
            if (input == "XX234") return "R_UR_U";
            if (input == "0X2X4") return "R_U_L";
            if (input == "012X4") return "L_UL_U_R";
            if (input == "0X234") return "L_U_UR_R";
            if (input == "01234") return "L_UL_U_UR_R";
            if (input == "XXXXX") return "Null";
            return "Null";
        }

        static string TranslateUpLeft(string input)
        {
            if (input == "0XX") return "L";
            if (input == "XX2") return "U";
            if (input == "0X2") return "U_L";
            if (input == "012") return "U_UL_L";
            if (input == "XXX") return "Null";
            return "Null";
        }

        static string TranslateLeft(string input)
        {
            if (input == "XX2XX") return "L";
            if (input == "0XXXX") return "D";
            if (input == "XXXX4") return "U";
            if (input == "0X2XX") return "L_D";
            if (input == "XX2X4") return "U_L";
            if (input == "0XXX4") return "D_U";
            if (input == "012XX") return "L_DL_D";
            if (input == "XX234") return "U_UL_L";
            if (input == "0X2X4") return "U_L_D";
            if (input == "012X4") return "D_DL_L_U";
            if (input == "0X234") return "D_L_UL_U";
            if (input == "01234") return "D_DL_L_UL_U";
            if (input == "XXXXX") return "Null";
            return "Null";
        }

        static string TranslateDownLeft(string input)
        {
            if (input == "0XX") return "D";
            if (input == "XX2") return "L";
            if (input == "0X2") return "L_D";
            if (input == "012") return "L_DL_D";
            if (input == "XXX") return "Null";
            return "Null";
        }


    }
}
