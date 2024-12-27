#include <cmath>

#include <sstream>
#include <limits>
#include <stdexcept>

#include "parser.h"
#include "config.h"

using std::isfinite;
namespace {
  // Numeric operations

  int unary_negate
  (parse_context* ctxt, expr_value_t *R, const expr_t * const *A)
  {
    *R = -std::get<double>(A[0]->value);
    return 0;
  }

  int unary_sin(parse_context* ctxt, expr_value_t *R, const expr_t * const *A)
  {
    *R = sin(std::get<double>(A[0]->value));
    return 0;
  }
  int unary_cos(parse_context* ctxt, expr_value_t *R, const expr_t * const *A)
  {
    *R = cos(std::get<double>(A[0]->value));
    return 0;
  }
  int unary_tan(parse_context* ctxt, expr_value_t *R, const expr_t * const *A)
  {
    *R = tan(std::get<double>(A[0]->value));
    return 0;
  }
  int unary_asin(parse_context* ctxt, expr_value_t *R, const expr_t * const *A)
  {
    *R = asin(std::get<double>(A[0]->value));
    return 0;
  }
  int unary_acos(parse_context* ctxt, expr_value_t *R, const expr_t * const *A)
  {
    *R = acos(std::get<double>(A[0]->value));
    return 0;
  }
  int unary_atan(parse_context* ctxt, expr_value_t *R, const expr_t * const *A)
  {
    *R = atan(std::get<double>(A[0]->value));
    return 0;
  }

  int unary_deg2rad
  (parse_context* ctxt, expr_value_t *R, const expr_t * const *A)
  {
    double val = std::get<double>(A[0]->value);
    val *= M_PI/180.0;
    *R = val;
    return 0;
  }
  int unary_rad2deg(parse_context* ctxt, expr_value_t *R, const expr_t * const *A)
  {
    double val = std::get<double>(A[0]->value);
    val *= 180.0/M_PI;
    *R = val;
    return 0;
  }

  int binary_add(parse_context* ctxt, expr_value_t *R, const expr_t * const *A)
  {
    *R = std::get<double>(A[0]->value)+std::get<double>(A[1]->value);
    return 0;
  }
  int binary_sub(parse_context* ctxt, expr_value_t *R, const expr_t * const *A)
  {
    *R = std::get<double>(A[0]->value)-std::get<double>(A[1]->value);
    return 0;
  }
  int binary_mult(parse_context* ctxt, expr_value_t *R, const expr_t * const *A)
  {
    *R = std::get<double>(A[0]->value)*std::get<double>(A[1]->value);
    return 0;
  }
  int binary_div(parse_context* ctxt, expr_value_t *R, const expr_t * const *A)
  {
    double result = std::get<double>(A[0]->value)/std::get<double>(A[1]->value);
    if(!isfinite(result)) {
      ctxt->last_error = "division results in non-finite value";
      return 1;
    }
    *R = result;
    return 0;
  }

  // beamline operations

  int unary_bl_negate
  (parse_context* ctxt, expr_value_t *R, const expr_t * const *A)
  {
    // reverse the order of the beamline
    const std::vector<std::string>&
      line = std::get<std::vector<std::string> >(A[0]->value);
    std::vector<std::string> ret(line.size());
    std::copy(line.rbegin(),
              line.rend(),
              ret.begin());
    *R = ret;
    return 0;
  }

  template<int MULT, int LINE>
  int binary_bl_mult
  (parse_context* ctxt, expr_value_t *R, const expr_t * const *A)
  {
    // multiple of scale * beamline repeats the beamline 'scale' times
    assert(A[MULT]->etype==gf_expr_number);
    assert(A[LINE]->etype==gf_expr_line);

    double factor = std::get<double>(A[MULT]->value);

    if(factor<0.0 || factor>std::numeric_limits<unsigned>::max()) {
      ctxt->last_error =
	"beamline scale by negative value or out of range value";
      return 1;
    }
    unsigned factori = (unsigned)factor;

    const std::vector<std::string>&
      line = std::get<std::vector<std::string> >(A[LINE]->value);

    std::vector<std::string> ret(line.size()*factori);

    if(factori>0)
      {
        std::vector<std::string>::iterator outi = ret.begin();

        while(factori--)
	  outi = std::copy(line.begin(),
			   line.end(),
			   outi);
      }
    *R = ret;
    return 0;
  }

  int unary_parse(parse_context* ctxt, expr_value_t *R, const expr_t * const *A)
  {
    using namespace std::filesystem;
    assert(A[0]->etype==gf_expr_string);
#warning "boost Canonical included current working directory ctxt->cwd"
    path name(canonical(std::get<std::string>(A[0]->value)));

    GFParser P;
    P.setPrinter(ctxt->printer);

    std::shared_ptr<Config> ret(P.parse_file(name.string().c_str()));
    *R = ret;
    return 0;
  }

  int unary_file(parse_context* ctxt, expr_value_t *R, const expr_t * const *A)
  {
    using namespace std::filesystem;
    const std::string& inp = std::get<std::string>(A[0]->value);
#warning "boost Canonical included current working directory ctxt->cwd"
    path ret(canonical(inp));

    if(!exists(ret)) {
      std::ostringstream strm;
      strm<<"\""<<ret<<"\" does not exist";
      ctxt->last_error = strm.str();
      return 1;
    }

    *R = ret.string();
    return 0;
  }

#ifdef USE_HDF5
  int unary_h5file
  (parse_context* ctxt, expr_value_t *R, const expr_t * const *A)
  {
    using namespace std::filesystem;
    const std::string& inp = std::get<std::string>(A[0]->value);

    /* The provided spec may contain both file path and group(s)
     * seperated by '/' which is ambigious as the file path
     * may contain '/' as well...
     * so do as h5ls does and strip off from the right hand side until
     * and try to open while '/' remain.
     */
    size_t sep = inp.npos;

    while(true) {
      sep = inp.find_last_of('/', sep-1);

      path fname(absolute(inp.substr(0, sep), ctxt->cwd));

      if(exists(fname)) {
	*R = canonical(fname).string() + inp.substr(sep);
	return 0;
      } else if(sep==inp.npos) {
	break;
      }
    }

    std::ostringstream strm;
    strm<<"\""<<inp<<"\" does not exist";
    ctxt->last_error = strm.str();
    return 1;
  }
#endif

} // namespace

parse_context::parse_context(const char *path)
  :last_line(0), printer(NULL), error_scratch(300), scanner(NULL)
{
  if(path)
    cwd = std::filesystem::canonical(path);
  else
    cwd = std::filesystem::current_path();
  addop("-", &unary_negate, gf_expr_number, 1, gf_expr_number);

  addop("sin", &unary_sin, gf_expr_number, 1, gf_expr_number);
  addop("cos", &unary_cos, gf_expr_number, 1, gf_expr_number);
  addop("tan", &unary_tan, gf_expr_number, 1, gf_expr_number);
  addop("asin",&unary_asin,gf_expr_number, 1, gf_expr_number);
  addop("acos",&unary_acos,gf_expr_number, 1, gf_expr_number);
  addop("atan",&unary_atan,gf_expr_number, 1, gf_expr_number);
  // aliases to capture legacy behavour :P
  addop("arcsin",&unary_asin,gf_expr_number, 1, gf_expr_number);
  addop("arccos",&unary_acos,gf_expr_number, 1, gf_expr_number);
  addop("arctan",&unary_atan,gf_expr_number, 1, gf_expr_number);

  addop("deg2rad",&unary_deg2rad,gf_expr_number, 1, gf_expr_number);
  addop("rad2deg",&unary_rad2deg,gf_expr_number, 1, gf_expr_number);

  addop("+", &binary_add, gf_expr_number, 2, gf_expr_number, gf_expr_number);
  addop("-", &binary_sub, gf_expr_number, 2, gf_expr_number, gf_expr_number);
  addop("*", &binary_mult,gf_expr_number, 2, gf_expr_number, gf_expr_number);
  addop("/", &binary_div, gf_expr_number, 2, gf_expr_number, gf_expr_number);

  addop("-", &unary_bl_negate, gf_expr_line, 1, gf_expr_line);

  addop("*", &binary_bl_mult<0,1>, gf_expr_line, 2, gf_expr_number, gf_expr_line);
  addop("*", &binary_bl_mult<1,0>, gf_expr_line, 2, gf_expr_line, gf_expr_number);

  addop("parse", &unary_parse, gf_expr_config, 1, gf_expr_string);
  addop("file", &unary_file, gf_expr_string, 1, gf_expr_string);
  addop("dir", &unary_file, gf_expr_string, 1, gf_expr_string);
#ifdef USE_HDF5
  addop("h5file", &unary_h5file, gf_expr_string, 1, gf_expr_string);
#endif
}

parse_context::~parse_context()
{
}
