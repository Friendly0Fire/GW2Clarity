using System.Globalization;
using CommandLine;
using System.IO;
using DirectXTexNet;
using System.Text;
using static System.Net.Mime.MediaTypeNames;
using System.Drawing;
using System;
using System.Linq;

class Program
{
    public class Options
    {
        [Option('d', "directory", Required = false, SetName = "dirmode", HelpText = "Specifies input directory.")]
        public string InputDirectory { get; set; }

        [Option('f', "files", Required = false, SetName = "filemode", Separator = ',', HelpText = "Comma-separated list of input files.")]
        public IEnumerable<string> InputFiles { get; set; }

        [Option('t', "font", Required = false, SetName = "fontmode")]
        public string Font { get; set; }

        [Option('s', "fontsize", Required = false, SetName = "fontmode", Default = 20)]
        public float FontSize { get; set; }

        [Option('n', "fontcount", Required = false, SetName = "fontmode")]
        public int FontCount { get; set; }

        [Option("bold", Required = false, SetName = "fontmode")]
        public bool FontBold { get; set; }

        [Option("italic", Required = false, SetName = "fontmode")]
        public bool FontItalic { get; set; }

        [Option("shadow", Required = false, SetName = "fontmode")]
        public int FontShadow { get; set; }

        [Option('s', "size", Required = false, Default = 0, HelpText = "Size of individual atlas elements in pixels.")]
        public int Size { get; set; }

        [Option('b', "border", Required = false, Default = 1, HelpText = "Width of the border around each element in pixels.")]
        public int Border { get; set; }

        [Option('o', "output", Required = true, HelpText = "Specifies output file name.")]
        public string OutputFile { get; set; }

        [Option('x', "format", Required = false, HelpText = "Specifies output format.")]
        public DXGI_FORMAT OutputFormat { get; set; } = DXGI_FORMAT.UNKNOWN;

        [Option('v', "verbose", Required = false, HelpText = "Set output to verbose messages.")]
        public bool Verbose { get; set; }
    }

    private static string[] SupportedExtensions =
    {
        ".dds",
        ".tga",
        ".bmp",
        ".jpg",
        ".jpeg",
        ".png"
    };

    public static void Main(string[] args)
    {
        Thread.CurrentThread.CurrentCulture = CultureInfo.InvariantCulture;
        
        Console.WriteLine("Loading Atlas Tool...");
        _ = Parser.Default.ParseArguments<Options>(args)
            .WithParsed(o =>
            {
                if (o.Font == null)
                {
                    o.InputDirectory = Path.GetFullPath(o.InputDirectory);
                    var inputFiles = o.InputFiles.Select(Path.GetFullPath).ToList();
                    IEnumerable<string> files;
                    if (inputFiles.Any())
                    {
                        files = inputFiles.Where(x => SupportedExtensions.Contains(Path.GetExtension(x).ToLowerInvariant()));
                    }
                    else
                        files = Directory.EnumerateFiles(o.InputDirectory);
                    files = files.OrderBy(Path.GetFileName);

                    if (o.Verbose)
                    {
                        Console.WriteLine("Using input files:");
                        foreach (string file in files)
                            Console.WriteLine($"\t{file}");
                    }

                    MakeAtlas(o.OutputFile, files, o.Size, o.Border, o.OutputFormat, o.Verbose);
                }
                else
                {
                    FontStyle style = FontStyle.Regular;
                    if (o.FontBold)
                        style |= FontStyle.Bold;
                    if (o.FontItalic)
                        style |= FontStyle.Italic;

                    MakeAtlas(o.OutputFile, o.Font, o.FontSize, o.FontCount, o.FontShadow, style, o.Size, o.Border, o.OutputFormat, o.Verbose);
                }

            });
    }


    private static unsafe void BitmapToDXTex(Bitmap bmp, IntPtr outPixels, IntPtr width, IntPtr y)
    {
        var outPtr = (float*)outPixels.ToPointer();

        for (int x = 0; x < width.ToInt32(); ++x)
        {
            var c = bmp.GetPixel(x, y.ToInt32());
            outPtr[x * 4] = c.R / 255.0f;
            outPtr[x * 4 + 1] = c.G / 255.0f;
            outPtr[x * 4 + 2] = c.B / 255.0f;
            outPtr[x * 4 + 3] = c.A / 255.0f;
        }
    }

    private static void MakeAtlas(string outputPath, string fontName, float fontSize, int fontCount, int fontShadow, FontStyle fontStyle, int size, int border, DXGI_FORMAT outFormat, bool verbose)
    {
        var texHelper = TexHelper.Instance;
        if (verbose)
            Console.WriteLine($"Using atlas element size: {size + border * 2}x{size + border * 2}.");

        int sizeWithBorders = size + border * 2;
        int squareEdge = (int)Math.Ceiling(Math.Sqrt(fontCount)) * sizeWithBorders;
        int baseSquareEdge = squareEdge;
        if (squareEdge % 4 != 0)
            squareEdge = ((squareEdge / 4) + 1) * 4;
        if (verbose)
            Console.WriteLine($"Final texture size: {squareEdge}x{squareEdge}.");

        StringBuilder sidecar = new StringBuilder();

        float uvSide = (float)(size + border) / (float)squareEdge;
        _ = sidecar.AppendLine($"{{ \"\", {{ {uvSide}, {uvSide} }} }},");

        var output = texHelper.Initialize2D(DXGI_FORMAT.R8G8B8A8_UNORM, squareEdge, squareEdge, 1, 0, CP_FLAGS.NONE);

        var outputBitmap = new Bitmap(squareEdge, squareEdge);
        var font = new Font(fontName, fontSize, fontStyle);
        var foreBrush = new SolidBrush(Color.White);
        var backBrush = new SolidBrush(Color.Black);

        Graphics g = Graphics.FromImage(outputBitmap);
        int x = border, y = border;
        for (int i = 0; i < fontCount; ++i)
        {
            var strDims = g.MeasureString($"{i}", font);
            int xOff = x + size - (int)strDims.Width;
            int yOff = y + size - (int)strDims.Height;
            if(fontShadow > 0)
                g.DrawString($"{i}", font, backBrush, new PointF(xOff + fontShadow, yOff + fontShadow));
            g.DrawString($"{i}", font, foreBrush, new PointF(xOff, yOff));

            _ = sidecar.AppendLine($"{{ \"{i.ToString(new string('0', fontCount.ToString().Length))}\", {{ {((float)x - border * 0.5f) / squareEdge}, {((float)y - border * 0.5f) / squareEdge} }} }},");

            x += sizeWithBorders;
            if (x >= baseSquareEdge)
            {
                x = border;
                y += sizeWithBorders;
            }
        }

        output = output.TransformImage(0, (IntPtr outPixels, IntPtr inPixels, IntPtr width, IntPtr y) => BitmapToDXTex(outputBitmap, outPixels, width, y));

        output = output.GenerateMipMaps(TEX_FILTER_FLAGS.DEFAULT, 0);
        var finalOutput = texHelper.IsCompressed(outFormat) ? output.Compress(outFormat, TEX_COMPRESS_FLAGS.DEFAULT, 0.5f) : output;

        finalOutput.SaveToDDSFile(DDS_FLAGS.NONE, outputPath);

        File.WriteAllText(Path.ChangeExtension(outputPath, ".inc"), sidecar.ToString());
    }

    private static void MakeAtlas(string outputPath, IEnumerable<string> inputPaths, int size, int border, DXGI_FORMAT outFormat, bool verbose)
    {
        var texHelper = TexHelper.Instance;
        IEnumerable<ScratchImage?> images = inputPaths.Select(f => {
            if (Path.GetExtension(f) == ".dds")
                return texHelper.LoadFromDDSFile(f, DDS_FLAGS.NONE);
            else if (Path.GetExtension(f) == ".tga")
                return texHelper.LoadFromTGAFile(f);
            else
            {
                try
                {
                    return texHelper.LoadFromWICFile(f, WIC_FLAGS.NONE);
                }
                catch(Exception)
                {
                    return null;
                }
            }
            }).Where(x => x != null);

        if(size == 0)
        {
            size = images.Select(i => i.GetMetadata().Width).Max();
            if(verbose)
                Console.WriteLine($"Detected atlas element size: {size + border * 2}x{size + border * 2}.");
        }
        int sizeWithBorders = size + border * 2;
        int squareEdge = (int)Math.Ceiling(Math.Sqrt(images.Count())) * sizeWithBorders;
        int baseSquareEdge = squareEdge;
        if(squareEdge % 4 != 0)
            squareEdge = ((squareEdge / 4) + 1) * 4;
        if (verbose)
            Console.WriteLine($"Final texture size: {squareEdge}x{squareEdge}.");

        StringBuilder sidecar = new StringBuilder();

        float uvSide = (float)(size + border) / (float)squareEdge;
        _ = sidecar.AppendLine($"{{ \"\", {{ {uvSide}, {uvSide} }} }},");

        var output = texHelper.Initialize2D(DXGI_FORMAT.R8G8B8A8_UNORM, squareEdge, squareEdge, 1, 0, CP_FLAGS.NONE);
        var output0 = output.GetImage(0);
        int x = border, y = border;
        foreach(var ip in images.Zip(inputPaths))
        {
            var i = ip.First;
            var p = ip.Second;

            DirectXTexNet.Image i0;
            if (texHelper.IsCompressed(i.GetMetadata().Format))
                i0 = i.Decompress(DXGI_FORMAT.R8G8B8A8_UNORM).GetImage(0);
            else
                i0 = i.GetImage(0);

            int xOff = x;
            int yOff = y;
            
            if (i0.Width < size)
                xOff += (size - i0.Width) / 2;
            if (i0.Height < size)
                yOff += (size - i0.Height) / 2;

            texHelper.CopyRectangle(i0, 0, 0, i0.Width, i0.Height, output0, TEX_FILTER_FLAGS.DEFAULT, xOff, yOff);

            _ = sidecar.AppendLine($"{{ \"{Path.GetFileNameWithoutExtension(p).ToLowerInvariant()}\", {{ {((float)x - border * 0.5f) / squareEdge}, {((float)y - border * 0.5f) / squareEdge} }} }},");

            x += sizeWithBorders;
            if(x >= baseSquareEdge)
            { 
                x = border;
                y += sizeWithBorders;
            }
        }

        var format = outFormat == DXGI_FORMAT.UNKNOWN ? images.First().GetMetadata().Format : outFormat;

        output = output.GenerateMipMaps(TEX_FILTER_FLAGS.DEFAULT, 0);
        var finalOutput = texHelper.IsCompressed(format) ? output.Compress(format, TEX_COMPRESS_FLAGS.DEFAULT, 0.5f) : output;

        finalOutput.SaveToDDSFile(DDS_FLAGS.NONE, outputPath);

        File.WriteAllText(Path.ChangeExtension(outputPath, ".inc"), sidecar.ToString());
    }
}


